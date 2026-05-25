#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spore.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SYS_SPORE_SET_BUDGET 0x4004
#define SYS_SPORE_APPLY_POLICY 0x4005
#define SYS_SPORE_SHUTDOWN 0x4006

enum {
  UNIT_CAP = 48,
  DEP_CAP = 12,
  ARG_CAP = 24,
  LOG_LINE = 256,
  LOG_MEM = 2048,
  START_LIMIT_HISTORY = 8,
};

enum unit_type {
  UNIT_SERVICE,
  UNIT_TARGET,
  UNIT_TIMER,
  UNIT_PATH,
  UNIT_SOCKET,
  UNIT_MOUNT,
};

enum service_type {
  SERVICE_SIMPLE,
  SERVICE_FORKING,
  SERVICE_ONESHOT,
  SERVICE_NOTIFY,
  SERVICE_IDLE,
};

enum unit_state {
  STATE_INACTIVE,
  STATE_ACTIVATING,
  STATE_ACTIVE,
  STATE_DEACTIVATING,
  STATE_FAILED,
};

enum restart_policy {
  RESTART_NO,
  RESTART_ON_SUCCESS,
  RESTART_ON_FAILURE,
  RESTART_ON_ABNORMAL,
  RESTART_ON_WATCHDOG,
  RESTART_ALWAYS,
};

struct string_list {
  char items[DEP_CAP][64];
  int count;
};

struct unit {
  bool used;
  enum unit_type type;
  enum service_type service_type;
  enum unit_state state;
  enum restart_policy restart;
  char name[64];
  char description[96];
  char exec_start[192];
  char exec_start_pre[192];
  char exec_start_post[192];
  char exec_stop[192];
  char exec_reload[192];
  char capability[64];
  char user[32];
  char standard_output[96];
  char standard_error[96];
  char wanted_by[64];
  char unit_to_activate[64];
  struct string_list requires;
  struct string_list wants;
  struct string_list requisite;
  struct string_list binds_to;
  struct string_list part_of;
  struct string_list conflicts;
  struct string_list after;
  struct string_list before;
  pid_t pid;
  int status;
  int journal_fd;
  bool journal_err;
  time_t active_since;
  time_t inactive_since;
  time_t last_watchdog;
  int restart_sec;
  int watchdog_sec;
  int timeout_stop_sec;
  int start_limit_interval;
  int start_limit_burst;
  time_t starts[START_LIMIT_HISTORY];
  int start_count;
  int on_boot_sec;
  int on_unit_active_sec;
  int on_unit_inactive_sec;
  int on_calendar_minute;
  uint64_t cpu_budget_ticks;
  uint64_t memory_pages;
  bool persistent;
  time_t next_fire;
  char log_mem[LOG_MEM];
  size_t log_len;
  char fail_reason[64];
};

static struct unit units[UNIT_CAP];
static int control_fd = -1;
static bool shutting_down;
static const char *start_stack[UNIT_CAP];
static int start_depth;
static bool defer_console_start = true;

static const char *state_name(enum unit_state state) {
  switch (state) {
  case STATE_INACTIVE:
    return "inactive";
  case STATE_ACTIVATING:
    return "activating";
  case STATE_ACTIVE:
    return "active";
  case STATE_DEACTIVATING:
    return "deactivating";
  case STATE_FAILED:
    return "failed";
  }
  return "?";
}

static const char *type_name(enum unit_type type) {
  switch (type) {
  case UNIT_SERVICE:
    return "service";
  case UNIT_TARGET:
    return "target";
  case UNIT_TIMER:
    return "timer";
  case UNIT_PATH:
    return "path";
  case UNIT_SOCKET:
    return "socket";
  case UNIT_MOUNT:
    return "mount";
  }
  return "?";
}

static void copy_text(char *dst, size_t cap, const char *src) {
  if (cap == 0) { return; }
  snprintf(dst, cap, "%s", src == NULL ? "" : src);
}

static char *trim(char *s) {
  while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
    ++s;
  }
  char *end = s + strlen(s);
  while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
    *--end = '\0';
  }
  return s;
}

static bool ends_with(const char *s, const char *suffix) {
  size_t a = strlen(s);
  size_t b = strlen(suffix);
  return a >= b && strcmp(s + a - b, suffix) == 0;
}

static void ensure_dir(const char *path) {
  char tmp[160];
  copy_text(tmp, sizeof(tmp), path);
  for (char *p = tmp + 1; *p != '\0'; ++p) {
    if (*p == '/') {
      *p = '\0';
      (void)mkdir(tmp, 0755);
      *p = '/';
    }
  }
  (void)mkdir(tmp, 0755);
}

static void append_response(char *out, size_t cap, const char *fmt, ...) {
  size_t len = strlen(out);
  if (len >= cap) { return; }
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(out + len, cap - len, fmt, ap);
  va_end(ap);
}

static int parse_seconds(const char *s) {
  char *end = NULL;
  long value = strtol(s, &end, 10);
  if (end != NULL && *end == 's') { ++end; }
  if (end == s || (end != NULL && *end != '\0') || value < 0 || value > 86400) { return 0; }
  return (int)value;
}

static void list_add_words(struct string_list *list, const char *value) {
  char copy[256];
  copy_text(copy, sizeof(copy), value);
  for (char *p = strtok(copy, " \t"); p != NULL && list->count < DEP_CAP; p = strtok(NULL, " \t")) {
    copy_text(list->items[list->count++], sizeof(list->items[0]), p);
  }
}

static bool list_has(const struct string_list *list, const char *name) {
  for (int i = 0; i < list->count; ++i) {
    if (streq(list->items[i], name)) { return true; }
  }
  return false;
}

static struct unit *unit_by_name(const char *name) {
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    if (units[i].used && streq(units[i].name, name)) { return &units[i]; }
  }
  return NULL;
}

static struct unit *unit_for_pid(pid_t pid) {
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    if (units[i].used && units[i].pid == pid) { return &units[i]; }
  }
  return NULL;
}

static struct unit *alloc_unit(const char *name) {
  struct unit *existing = unit_by_name(name);
  if (existing != NULL) { return existing; }
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    if (!units[i].used) {
      memset(&units[i], 0, sizeof(units[i]));
      units[i].used = true;
      copy_text(units[i].name, sizeof(units[i].name), name);
      units[i].type = UNIT_SERVICE;
      units[i].service_type = SERVICE_SIMPLE;
      units[i].state = STATE_INACTIVE;
      units[i].restart = RESTART_NO;
      units[i].journal_fd = -1;
      units[i].restart_sec = 1;
      units[i].timeout_stop_sec = 3;
      units[i].start_limit_interval = 10;
      units[i].start_limit_burst = 3;
      units[i].on_calendar_minute = -1;
      copy_text(units[i].standard_output, sizeof(units[i].standard_output), "journal");
      copy_text(units[i].standard_error, sizeof(units[i].standard_error), "journal");
      return &units[i];
    }
  }
  return NULL;
}

static void infer_unit_type(struct unit *unit) {
  if (ends_with(unit->name, ".target")) {
    unit->type = UNIT_TARGET;
  } else if (ends_with(unit->name, ".timer")) {
    unit->type = UNIT_TIMER;
  } else if (ends_with(unit->name, ".path")) {
    unit->type = UNIT_PATH;
  } else if (ends_with(unit->name, ".socket")) {
    unit->type = UNIT_SOCKET;
  } else if (ends_with(unit->name, ".mount")) {
    unit->type = UNIT_MOUNT;
  } else {
    unit->type = UNIT_SERVICE;
  }
}

static void set_key(struct unit *unit, const char *section, const char *key, const char *value) {
  if (streq(section, "Unit")) {
    if (streq(key, "Description")) {
      copy_text(unit->description, sizeof(unit->description), value);
    } else if (streq(key, "Requires")) {
      list_add_words(&unit->requires, value);
    } else if (streq(key, "Wants")) {
      list_add_words(&unit->wants, value);
    } else if (streq(key, "Requisite")) {
      list_add_words(&unit->requisite, value);
    } else if (streq(key, "BindsTo")) {
      list_add_words(&unit->binds_to, value);
    } else if (streq(key, "PartOf")) {
      list_add_words(&unit->part_of, value);
    } else if (streq(key, "Conflicts")) {
      list_add_words(&unit->conflicts, value);
    } else if (streq(key, "After")) {
      list_add_words(&unit->after, value);
    } else if (streq(key, "Before")) {
      list_add_words(&unit->before, value);
    }
  } else if (streq(section, "Service")) {
    if (streq(key, "Type")) {
      if (streq(value, "oneshot")) {
        unit->service_type = SERVICE_ONESHOT;
      } else if (streq(value, "forking")) {
        unit->service_type = SERVICE_FORKING;
      } else if (streq(value, "notify")) {
        unit->service_type = SERVICE_NOTIFY;
      } else if (streq(value, "idle")) {
        unit->service_type = SERVICE_IDLE;
      } else {
        unit->service_type = SERVICE_SIMPLE;
      }
    } else if (streq(key, "ExecStart")) {
      copy_text(unit->exec_start, sizeof(unit->exec_start), value);
    } else if (streq(key, "ExecStartPre")) {
      copy_text(unit->exec_start_pre, sizeof(unit->exec_start_pre), value);
    } else if (streq(key, "ExecStartPost")) {
      copy_text(unit->exec_start_post, sizeof(unit->exec_start_post), value);
    } else if (streq(key, "ExecStop")) {
      copy_text(unit->exec_stop, sizeof(unit->exec_stop), value);
    } else if (streq(key, "ExecReload")) {
      copy_text(unit->exec_reload, sizeof(unit->exec_reload), value);
    } else if (streq(key, "Restart")) {
      if (streq(value, "on-success")) {
        unit->restart = RESTART_ON_SUCCESS;
      } else if (streq(value, "on-failure")) {
        unit->restart = RESTART_ON_FAILURE;
      } else if (streq(value, "on-abnormal")) {
        unit->restart = RESTART_ON_ABNORMAL;
      } else if (streq(value, "on-watchdog")) {
        unit->restart = RESTART_ON_WATCHDOG;
      } else if (streq(value, "always")) {
        unit->restart = RESTART_ALWAYS;
      } else {
        unit->restart = RESTART_NO;
      }
    } else if (streq(key, "RestartSec")) {
      unit->restart_sec = parse_seconds(value);
    } else if (streq(key, "StartLimitIntervalSec")) {
      unit->start_limit_interval = parse_seconds(value);
    } else if (streq(key, "StartLimitBurst")) {
      unit->start_limit_burst = atoi(value);
    } else if (streq(key, "WatchdogSec")) {
      unit->watchdog_sec = parse_seconds(value);
    } else if (streq(key, "TimeoutStopSec")) {
      unit->timeout_stop_sec = parse_seconds(value);
    } else if (streq(key, "Capability")) {
      copy_text(unit->capability, sizeof(unit->capability), value);
    } else if (streq(key, "CPUBudget")) {
      unit->cpu_budget_ticks = (uint64_t)parse_seconds(value);
    } else if (streq(key, "MemoryPages")) {
      unit->memory_pages = strtoull(value, NULL, 10);
    } else if (streq(key, "User")) {
      copy_text(unit->user, sizeof(unit->user), value);
    } else if (streq(key, "StandardOutput")) {
      copy_text(unit->standard_output, sizeof(unit->standard_output), value);
    } else if (streq(key, "StandardError")) {
      copy_text(unit->standard_error, sizeof(unit->standard_error), value);
    }
  } else if (streq(section, "Timer")) {
    unit->type = UNIT_TIMER;
    if (streq(key, "OnBootSec")) {
      unit->on_boot_sec = parse_seconds(value);
    } else if (streq(key, "OnUnitActiveSec")) {
      unit->on_unit_active_sec = parse_seconds(value);
    } else if (streq(key, "OnUnitInactiveSec")) {
      unit->on_unit_inactive_sec = parse_seconds(value);
    } else if (streq(key, "OnCalendar")) {
      if (strchr(value, ':') != NULL) {
        const char *colon = strchr(value, ':');
        unit->on_calendar_minute = atoi(colon + 1);
      } else {
        unit->on_calendar_minute = atoi(value);
      }
    } else if (streq(key, "Persistent")) {
      unit->persistent = streq(value, "yes") || streq(value, "true");
    } else if (streq(key, "Unit")) {
      copy_text(unit->unit_to_activate, sizeof(unit->unit_to_activate), value);
    }
  } else if (streq(section, "Install")) {
    if (streq(key, "WantedBy") || streq(key, "RequiredBy")) {
      copy_text(unit->wanted_by, sizeof(unit->wanted_by), value);
    }
  }
}

static bool parse_unit_file(const char *path, const char *name) {
  FILE *f = fopen(path, "r");
  if (f == NULL) { return false; }
  struct unit *unit = alloc_unit(name);
  if (unit == NULL) {
    fclose(f);
    return false;
  }
  infer_unit_type(unit);
  char section[32] = "Unit";
  char line[512];
  while (fgets(line, sizeof(line), f) != NULL) {
    char *p = trim(line);
    if (*p == '\0' || *p == '#') { continue; }
    if (*p == '[') {
      char *end = strchr(p, ']');
      if (end != NULL) {
        *end = '\0';
        copy_text(section, sizeof(section), p + 1);
      }
      continue;
    }
    char *eq = strchr(p, '=');
    if (eq == NULL) { continue; }
    *eq = '\0';
    set_key(unit, section, trim(p), trim(eq + 1));
  }
  fclose(f);
  return true;
}

static void parse_builtin_console(void) {
  struct unit *console = alloc_unit("console.service");
  if (console == NULL) { return; }
  copy_text(console->description, sizeof(console->description), "interactive console shell");
  copy_text(console->exec_start, sizeof(console->exec_start), "/bin/msh");
  copy_text(console->user, sizeof(console->user), "spore");
  copy_text(console->standard_output, sizeof(console->standard_output), "console");
  copy_text(console->standard_error, sizeof(console->standard_error), "console");
  console->restart = RESTART_ALWAYS;
}

static void load_units(void) {
  memset(units, 0, sizeof(units));
  parse_builtin_console();
  DIR *dir = opendir("/etc/mycelium");
  if (dir == NULL) { return; }
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_name[0] == '.') { continue; }
    char path[160];
    snprintf(path, sizeof(path), "/etc/mycelium/%s", ent->d_name);
    (void)parse_unit_file(path, ent->d_name);
  }
  closedir(dir);
}

static void timestamp(char *out, size_t cap) {
  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  strftime(out, cap, "%Y-%m-%d %H:%M:%S", &tm);
}

static void journal_append(struct unit *unit, const char *fmt, ...) {
  if (unit == NULL) { return; }
  char msg[LOG_LINE];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  char ts[32];
  timestamp(ts, sizeof(ts));
  char line[LOG_LINE + 96];
  snprintf(line, sizeof(line), "%s %s: %s\n", ts, unit->name, msg);

  size_t n = strlen(line);
  if (n >= sizeof(unit->log_mem)) {
    n = sizeof(unit->log_mem) - 1;
    line[n] = '\0';
  }
  if (unit->log_len + n >= sizeof(unit->log_mem)) {
    size_t drop = unit->log_len + n - sizeof(unit->log_mem) + 1;
    if (drop < unit->log_len) {
      memmove(unit->log_mem, unit->log_mem + drop, unit->log_len - drop);
      unit->log_len -= drop;
    } else {
      unit->log_len = 0;
    }
  }
  memcpy(unit->log_mem + unit->log_len, line, n);
  unit->log_len += n;
  unit->log_mem[unit->log_len] = '\0';

  ensure_dir("/var/log/mycelium");
  char path[160];
  snprintf(path, sizeof(path), "/var/log/mycelium/%s.log", unit->name);
  FILE *f = fopen(path, "a");
  if (f != NULL) {
    fputs(line, f);
    fclose(f);
  }
}

static int split_args(char *cmd, char **argv, int cap) {
  int argc = 0;
  char *p = cmd;
  while (*p != '\0' && argc + 1 < cap) {
    while (*p == ' ' || *p == '\t') {
      ++p;
    }
    if (*p == '\0') { break; }
    if (*p == '"' || *p == '\'') {
      char quote = *p++;
      argv[argc++] = p;
      while (*p != '\0' && *p != quote) {
        ++p;
      }
    } else {
      argv[argc++] = p;
      while (*p != '\0' && *p != ' ' && *p != '\t') {
        ++p;
      }
    }
    if (*p != '\0') { *p++ = '\0'; }
  }
  argv[argc] = NULL;
  return argc;
}

static void redirect_output(const char *mode, int fd, int fallback_fd) {
  if (mode[0] == '\0' || streq(mode, "journal")) { return; }
  if (streq(mode, "console")) {
    dup2(fallback_fd, fd);
  } else if (streq(mode, "null")) {
    int nullfd = open("/dev/null", O_WRONLY);
    if (nullfd >= 0) {
      dup2(nullfd, fd);
      close(nullfd);
    }
  } else if (strncmp(mode, "file:", 5) == 0) {
    int out = open(mode + 5, O_CREAT | O_WRONLY | O_APPEND, 0666);
    if (out >= 0) {
      dup2(out, fd);
      close(out);
    }
  }
}

static void close_inherited_manager_fds(void) {
  /*
   * The kernel is single-core/run-to-completion, but forked services inherit
   * PID 1's accepted AF_UNIX control sockets unless we close them before exec.
   * Keep only stdio after all requested dup2() routing is complete.
   */
  for (int fd = 3; fd < 64; ++fd) {
    close(fd);
  }
}

static bool rate_limit_ok(struct unit *unit) {
  time_t now = time(NULL);
  int kept = 0;
  for (int i = 0; i < unit->start_count && i < START_LIMIT_HISTORY; ++i) {
    if (now - unit->starts[i] <= unit->start_limit_interval) { unit->starts[kept++] = unit->starts[i]; }
  }
  unit->start_count = kept;
  if (unit->start_limit_burst > 0 && kept >= unit->start_limit_burst) {
    copy_text(unit->fail_reason, sizeof(unit->fail_reason), "start-limit-hit");
    unit->state = STATE_FAILED;
    journal_append(unit, "start limit hit");
    return false;
  }
  if (unit->start_count < START_LIMIT_HISTORY) { unit->starts[unit->start_count++] = now; }
  return true;
}

static bool start_limit_exhausted(struct unit *unit) {
  if (unit->start_limit_burst <= 0) { return false; }
  time_t now = time(NULL);
  int kept = 0;
  for (int i = 0; i < unit->start_count && i < START_LIMIT_HISTORY; ++i) {
    if (now - unit->starts[i] <= unit->start_limit_interval) { unit->starts[kept++] = unit->starts[i]; }
  }
  unit->start_count = kept;
  return kept >= unit->start_limit_burst;
}

static int run_oneshot_command(struct unit *unit, const char *cmdline);

static int start_unit_name(const char *name, char *err, size_t err_cap);

static bool start_deps(struct unit *unit, char *err, size_t err_cap) {
  for (int i = 0; i < unit->conflicts.count; ++i) {
    struct unit *conflict = unit_by_name(unit->conflicts.items[i]);
    if (conflict != NULL && (conflict->state == STATE_ACTIVE || conflict->state == STATE_ACTIVATING)) {
      snprintf(err, err_cap, "%s conflicts with active %s\n", unit->name, conflict->name);
      return false;
    }
  }
  for (int i = 0; i < unit->requisite.count; ++i) {
    struct unit *requisite = unit_by_name(unit->requisite.items[i]);
    if (requisite == NULL || (requisite->state != STATE_ACTIVE && requisite->state != STATE_ACTIVATING)) {
      snprintf(err, err_cap, "%s requisite %s is not active\n", unit->name, unit->requisite.items[i]);
      return false;
    }
  }
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    if (units[i].used && list_has(&units[i].before, unit->name) &&
        (units[i].state == STATE_INACTIVE || units[i].state == STATE_FAILED)) {
      (void)start_unit_name(units[i].name, err, err_cap);
    }
  }
  for (int i = 0; i < unit->requires.count; ++i) {
    if (start_unit_name(unit->requires.items[i], err, err_cap) != 0) { return false; }
  }
  for (int i = 0; i < unit->binds_to.count; ++i) {
    if (start_unit_name(unit->binds_to.items[i], err, err_cap) != 0) { return false; }
  }
  for (int i = 0; i < unit->wants.count; ++i) {
    (void)start_unit_name(unit->wants.items[i], err, err_cap);
  }
  for (int i = 0; i < unit->after.count; ++i) {
    struct unit *after = unit_by_name(unit->after.items[i]);
    if (after != NULL && after->state == STATE_INACTIVE) { (void)start_unit_name(after->name, err, err_cap); }
  }
  return true;
}

static void arm_timer(struct unit *unit) {
  time_t now = time(NULL);
  if (unit->on_boot_sec > 0 && unit->next_fire == 0) {
    unit->next_fire = now + unit->on_boot_sec;
  } else if (unit->on_unit_active_sec > 0) {
    unit->next_fire = now + unit->on_unit_active_sec;
  } else if (unit->on_calendar_minute >= 0) {
    struct tm tm;
    localtime_r(&now, &tm);
    tm.tm_sec = 0;
    tm.tm_min = unit->on_calendar_minute;
    time_t candidate = mktime(&tm);
    if (candidate <= now) { candidate += 3600; }
    unit->next_fire = candidate;
  }
}

static int spawn_service(struct unit *unit) {
  if (unit->exec_start[0] == '\0') { return -1; }
  if (!rate_limit_ok(unit)) { return -1; }

  int journal_pipe[2] = {-1, -1};
  bool journal_out = streq(unit->standard_output, "journal") || streq(unit->standard_error, "journal");
  if (journal_out && pipe(journal_pipe) != 0) { return -1; }

  pid_t pid = fork();
  if (pid < 0) {
    if (journal_pipe[0] >= 0) {
      close(journal_pipe[0]);
      close(journal_pipe[1]);
    }
    return -1;
  }
  if (pid == 0) {
    if (journal_pipe[1] >= 0) {
      if (streq(unit->standard_output, "journal")) { dup2(journal_pipe[1], STDOUT_FILENO); }
      if (streq(unit->standard_error, "journal")) { dup2(journal_pipe[1], STDERR_FILENO); }
      close(journal_pipe[0]);
      close(journal_pipe[1]);
    }
    redirect_output(unit->standard_output, STDOUT_FILENO, STDOUT_FILENO);
    redirect_output(unit->standard_error, STDERR_FILENO, STDERR_FILENO);

    if (unit->capability[0] != '\0') {
      if (syscall(SYS_SPORE_APPLY_POLICY, unit->capability) != 0) { _exit(126); }
    }
    if (unit->cpu_budget_ticks != 0) { (void)syscall(SYS_SPORE_SET_BUDGET, 0, unit->cpu_budget_ticks); }
    if (unit->memory_pages != 0 && unit->capability[0] == '\0') {
      char mem_cap[64];
      snprintf(mem_cap, sizeof(mem_cap), "mem:%llu", (unsigned long long)unit->memory_pages);
      (void)syscall(SYS_SPORE_APPLY_POLICY, mem_cap);
    }
    if (unit->user[0] != '\0') {
      struct user_entry user;
      if (user_by_name(unit->user, &user)) {
        (void)setgid(user.gid);
        (void)setuid(user.uid);
        (void)chdir(user.home);
        setenv("HOME", user.home, 1);
        setenv("USER", user.name, 1);
        setenv("LOGNAME", user.name, 1);
        setenv("SHELL", user.shell, 1);
      }
    }
    close_inherited_manager_fds();
    char copy[256];
    copy_text(copy, sizeof(copy), unit->exec_start);
    char *argv[ARG_CAP];
    split_args(copy, argv, ARG_CAP);
    execvp(argv[0], argv);
    perror(argv[0]);
    _exit(127);
  }

  if (journal_pipe[1] >= 0) { close(journal_pipe[1]); }
  unit->journal_fd = journal_pipe[0];
  unit->pid = pid;
  unit->status = 0;
  unit->state = unit->service_type == SERVICE_NOTIFY ? STATE_ACTIVATING : STATE_ACTIVE;
  unit->active_since = time(NULL);
  unit->last_watchdog = unit->active_since;
  copy_text(unit->fail_reason, sizeof(unit->fail_reason), "");
  journal_append(unit, "started pid=%d", (int)pid);
  return 0;
}

static int start_unit_name(const char *name, char *err, size_t err_cap) {
  struct unit *unit = unit_by_name(name);
  if (unit == NULL) {
    snprintf(err, err_cap, "%s: not found\n", name);
    return -1;
  }
  if (defer_console_start && streq(unit->name, "console.service")) { return 0; }
  for (int i = 0; i < start_depth; ++i) {
    if (streq(start_stack[i], name)) {
      snprintf(err, err_cap, "dependency cycle at %s\n", name);
      unit->state = STATE_FAILED;
      copy_text(unit->fail_reason, sizeof(unit->fail_reason), "dependency-cycle");
      journal_append(unit, "dependency cycle detected");
      return -1;
    }
  }
  if (unit->state == STATE_ACTIVE || unit->state == STATE_ACTIVATING) { return 0; }
  int result = 0;
  if (start_depth < UNIT_CAP) { start_stack[start_depth++] = unit->name; }
  if (!start_deps(unit, err, err_cap)) {
    result = -1;
    goto out;
  }
  if (unit->type == UNIT_TARGET) {
    unit->state = STATE_ACTIVE;
    unit->active_since = time(NULL);
    goto out;
  }
  if (unit->type == UNIT_TIMER) {
    unit->state = STATE_ACTIVE;
    unit->active_since = time(NULL);
    if (unit->unit_to_activate[0] == '\0') {
      char base[64];
      copy_text(base, sizeof(base), unit->name);
      char *dot = strrchr(base, '.');
      if (dot != NULL) { strcpy(dot, ".service"); }
      copy_text(unit->unit_to_activate, sizeof(unit->unit_to_activate), base);
    }
    arm_timer(unit);
    goto out;
  }
  if (unit->type != UNIT_SERVICE) {
    unit->state = STATE_ACTIVE;
    goto out;
  }

  if (unit->exec_start_pre[0] != '\0' && run_oneshot_command(unit, unit->exec_start_pre) != 0) {
    unit->state = STATE_FAILED;
    copy_text(unit->fail_reason, sizeof(unit->fail_reason), "start-pre-failed");
    result = -1;
    goto out;
  }
  if (unit->service_type == SERVICE_ONESHOT) {
    int rc = run_oneshot_command(unit, unit->exec_start);
    unit->status = rc << 8;
    unit->state = rc == 0 ? STATE_ACTIVE : STATE_FAILED;
    unit->active_since = time(NULL);
    if (unit->exec_start_post[0] != '\0') { (void)run_oneshot_command(unit, unit->exec_start_post); }
    result = rc == 0 ? 0 : -1;
    goto out;
  }
  result = spawn_service(unit);
out:
  if (start_depth > 0) { --start_depth; }
  return result;
}

static int run_oneshot_command(struct unit *unit, const char *cmdline) {
  if (cmdline == NULL || cmdline[0] == '\0') { return 0; }
  char copy[256];
  copy_text(copy, sizeof(copy), cmdline);
  char *argv[ARG_CAP];
  split_args(copy, argv, ARG_CAP);
  pid_t pid = fork();
  if (pid < 0) { return 1; }
  if (pid == 0) {
    if (unit->capability[0] != '\0') { (void)syscall(SYS_SPORE_APPLY_POLICY, unit->capability); }
    close_inherited_manager_fds();
    execvp(argv[0], argv);
    _exit(127);
  }
  int status = 0;
  (void)waitpid(pid, &status, 0);
  int rc = WIFEXITED(status) ? WEXITSTATUS(status) : 128;
  journal_append(unit, "oneshot '%s' exit=%d", cmdline, rc);
  return rc;
}

static void stop_unit(struct unit *unit) {
  if (unit == NULL || unit->state == STATE_INACTIVE) { return; }
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    if (!units[i].used || &units[i] == unit) { continue; }
    if (list_has(&units[i].part_of, unit->name) || list_has(&units[i].binds_to, unit->name)) { stop_unit(&units[i]); }
  }
  unit->state = STATE_DEACTIVATING;
  if (unit->exec_stop[0] != '\0') { (void)run_oneshot_command(unit, unit->exec_stop); }
  if (unit->pid > 0) {
    kill(unit->pid, SIGTERM);
    time_t deadline = time(NULL) + (unit->timeout_stop_sec > 0 ? unit->timeout_stop_sec : 1);
    int status = 0;
    while (time(NULL) <= deadline) {
      pid_t got = waitpid(unit->pid, &status, WNOHANG);
      if (got == unit->pid) {
        unit->pid = 0;
        break;
      }
    }
    if (unit->pid > 0) {
      kill(unit->pid, SIGKILL);
      (void)waitpid(unit->pid, &status, 0);
      unit->pid = 0;
    }
  }
  if (unit->journal_fd >= 0) {
    close(unit->journal_fd);
    unit->journal_fd = -1;
  }
  unit->state = STATE_INACTIVE;
  unit->inactive_since = time(NULL);
  journal_append(unit, "stopped");
}

static bool should_restart(struct unit *unit, int status) {
  bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  bool abnormal = WIFSIGNALED(status);
  switch (unit->restart) {
  case RESTART_NO:
    return false;
  case RESTART_ON_SUCCESS:
    return success;
  case RESTART_ON_FAILURE:
    return !success;
  case RESTART_ON_ABNORMAL:
    return abnormal;
  case RESTART_ON_WATCHDOG:
    return streq(unit->fail_reason, "watchdog");
  case RESTART_ALWAYS:
    return true;
  }
  return false;
}

static void reap_children(void) {
  int status = 0;
  for (;;) {
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid <= 0) { return; }
    struct unit *unit = unit_for_pid(pid);
    if (unit == NULL) { continue; }
    unit->pid = 0;
    unit->status = status;
    if (unit->journal_fd >= 0) {
      close(unit->journal_fd);
      unit->journal_fd = -1;
    }
    if (WIFEXITED(status)) {
      journal_append(unit, "exited status=%d", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      journal_append(unit, "signaled signal=%d", WTERMSIG(status));
    }
    bool restart = !shutting_down && should_restart(unit, status);
    if (restart) {
      if (start_limit_exhausted(unit)) {
        unit->state = STATE_FAILED;
        copy_text(unit->fail_reason, sizeof(unit->fail_reason), "start-limit-hit");
        journal_append(unit, "start limit hit");
        continue;
      }
      unit->state = STATE_INACTIVE;
      if (unit->restart_sec > 0) { sleep((unsigned)unit->restart_sec); }
      char err[128] = {0};
      if (start_unit_name(unit->name, err, sizeof(err)) != 0) { unit->state = STATE_FAILED; }
    } else {
      unit->state = WIFEXITED(status) && WEXITSTATUS(status) == 0 ? STATE_INACTIVE : STATE_FAILED;
      if (unit->state == STATE_FAILED && unit->fail_reason[0] == '\0') {
        copy_text(unit->fail_reason, sizeof(unit->fail_reason), "exit-failure");
      }
    }
  }
}

static void read_journal_fd(struct unit *unit) {
  char buf[192];
  ssize_t n = read(unit->journal_fd, buf, sizeof(buf) - 1);
  if (n <= 0) {
    close(unit->journal_fd);
    unit->journal_fd = -1;
    return;
  }
  buf[n] = '\0';
  char *line = strtok(buf, "\n");
  while (line != NULL) {
    if (strstr(line, "READY=1") != NULL && unit->state == STATE_ACTIVATING) { unit->state = STATE_ACTIVE; }
    if (strstr(line, "WATCHDOG=1") != NULL) { unit->last_watchdog = time(NULL); }
    journal_append(unit, "%s", line);
    line = strtok(NULL, "\n");
  }
}

static void check_timers(void) {
  time_t now = time(NULL);
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    struct unit *unit = &units[i];
    if (!unit->used || unit->type != UNIT_TIMER || unit->state != STATE_ACTIVE || unit->next_fire == 0 ||
        unit->next_fire > now) {
      continue;
    }
    char err[128] = {0};
    (void)start_unit_name(unit->unit_to_activate, err, sizeof(err));
    arm_timer(unit);
  }
}

static void check_watchdogs(void) {
  time_t now = time(NULL);
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    struct unit *unit = &units[i];
    if (!unit->used || unit->watchdog_sec <= 0 || unit->state != STATE_ACTIVE || unit->pid <= 0) { continue; }
    if (now - unit->last_watchdog > unit->watchdog_sec) {
      copy_text(unit->fail_reason, sizeof(unit->fail_reason), "watchdog");
      journal_append(unit, "watchdog missed");
      kill(unit->pid, SIGKILL);
    }
  }
}

static int next_poll_timeout_ms(void) {
  time_t now = time(NULL);
  int best = 1000;
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    struct unit *unit = &units[i];
    if (!unit->used || unit->type != UNIT_TIMER || unit->state != STATE_ACTIVE || unit->next_fire == 0) { continue; }
    int ms = unit->next_fire <= now ? 0 : (int)((unit->next_fire - now) * 1000);
    if (ms < best) { best = ms; }
  }
  return best;
}

static void write_unit_status(char *out, size_t cap, struct unit *unit) {
  if (unit == NULL) {
    append_response(out, cap, "not-found\n");
    return;
  }
  if (unit->state == STATE_FAILED) {
    append_response(out, cap, "%s: failed (%s)%s%s\n", unit->name, unit->fail_reason[0] ? unit->fail_reason : "failed",
                    unit->pid > 0 ? "; pid " : "", unit->pid > 0 ? "running" : "");
  } else {
    append_response(out, cap, "%s: %s (%s)%s%d%s%s\n", unit->name, state_name(unit->state),
                    unit->pid > 0 ? "running" : type_name(unit->type), unit->pid > 0 ? " since boot; pid " : "",
                    (int)unit->pid, unit->capability[0] ? "; cap=" : "", unit->capability);
  }
}

static void list_units(char *out, size_t cap) {
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    if (!units[i].used) { continue; }
    append_response(out, cap, "%-22s %-8s %s\n", units[i].name, state_name(units[i].state),
                    units[i].pid > 0 ? "running" : type_name(units[i].type));
  }
}

static void list_timers(char *out, size_t cap) {
  time_t now = time(NULL);
  for (size_t i = 0; i < UNIT_CAP; ++i) {
    struct unit *unit = &units[i];
    if (!unit->used || unit->type != UNIT_TIMER) { continue; }
    if (unit->next_fire > now) {
      append_response(out, cap, "%-16s +%lds     %s\n", unit->name, (long)(unit->next_fire - now),
                      unit->unit_to_activate);
    } else {
      append_response(out, cap, "%-16s waiting   %s\n", unit->name, unit->unit_to_activate);
    }
  }
}

static void list_dependencies(char *out, size_t cap, struct unit *unit, int depth) {
  if (unit == NULL || depth > 8) { return; }
  for (int i = 0; i < depth; ++i) {
    append_response(out, cap, "  ");
  }
  append_response(out, cap, "%s\n", unit->name);
  for (int i = 0; i < unit->requires.count; ++i) {
    list_dependencies(out, cap, unit_by_name(unit->requires.items[i]), depth + 1);
  }
  for (int i = 0; i < unit->wants.count; ++i) {
    list_dependencies(out, cap, unit_by_name(unit->wants.items[i]), depth + 1);
  }
}

static void cat_unit(char *out, size_t cap, const char *name) {
  char path[160];
  snprintf(path, sizeof(path), "/etc/mycelium/%s", name);
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    append_response(out, cap, "%s: not found\n", name);
    return;
  }
  char line[256];
  while (fgets(line, sizeof(line), f) != NULL) {
    append_response(out, cap, "%s", line);
  }
  fclose(f);
}

static void stop_all_reverse(char *out, size_t cap) {
  shutting_down = true;
  append_response(out, cap, "mycelium: stopping units in reverse order...");
  for (int i = UNIT_CAP - 1; i >= 0; --i) {
    if (units[i].used && units[i].state == STATE_ACTIVE && !streq(units[i].name, "shutdown.target")) {
      append_response(out, cap, " %s", units[i].name);
      stop_unit(&units[i]);
      append_response(out, cap, " stopped...");
    }
  }
  append_response(out, cap, " powering off\n");
}

static void handle_command(const char *cmdline, char *out, size_t cap) {
  char copy[256];
  copy_text(copy, sizeof(copy), cmdline);
  char *argv[ARG_CAP];
  int argc = split_args(copy, argv, ARG_CAP);
  if (argc == 0) {
    append_response(out, cap, "empty command\n");
    return;
  }
  if (streq(argv[0], "status")) {
    if (argc == 1) {
      int count = 0;
      int failed = 0;
      for (size_t i = 0; i < UNIT_CAP; ++i) {
        if (units[i].used) {
          ++count;
          if (units[i].state == STATE_FAILED) { ++failed; }
        }
      }
      append_response(out, cap, "mycelium: running, %d units, %d failed\n", count, failed);
    } else {
      write_unit_status(out, cap, unit_by_name(argv[1]));
    }
  } else if (streq(argv[0], "list-units")) {
    list_units(out, cap);
  } else if (streq(argv[0], "list-timers")) {
    list_timers(out, cap);
  } else if (streq(argv[0], "list-dependencies")) {
    list_dependencies(out, cap, unit_by_name(argc > 1 ? argv[1] : "multi-user.target"), 0);
  } else if (streq(argv[0], "start")) {
    if (argc < 2) {
      append_response(out, cap, "start: missing unit\n");
    } else {
      char err[160] = {0};
      if (start_unit_name(argv[1], err, sizeof(err)) == 0) {
        append_response(out, cap, "%s started\n", argv[1]);
      } else {
        append_response(out, cap, "%s", err[0] ? err : "start failed\n");
      }
    }
  } else if (streq(argv[0], "stop")) {
    stop_unit(unit_by_name(argc > 1 ? argv[1] : ""));
    append_response(out, cap, "%s stopped\n", argc > 1 ? argv[1] : "");
  } else if (streq(argv[0], "restart")) {
    if (argc > 1) {
      stop_unit(unit_by_name(argv[1]));
      char err[160] = {0};
      (void)start_unit_name(argv[1], err, sizeof(err));
      append_response(out, cap, "%s restarted\n", argv[1]);
    }
  } else if (streq(argv[0], "reload")) {
    struct unit *unit = unit_by_name(argc > 1 ? argv[1] : "");
    if (unit != NULL && unit->exec_reload[0] != '\0') { (void)run_oneshot_command(unit, unit->exec_reload); }
    append_response(out, cap, "%s reloaded\n", argc > 1 ? argv[1] : "");
  } else if (streq(argv[0], "daemon-reload")) {
    load_units();
    append_response(out, cap, "daemon reloaded\n");
  } else if (streq(argv[0], "logs")) {
    int arg = 1;
    if (argc > 1 && streq(argv[1], "-f")) { arg = 2; }
    struct unit *unit = unit_by_name(arg < argc ? argv[arg] : "");
    if (unit == NULL) {
      append_response(out, cap, "logs: unit not found\n");
    } else {
      append_response(out, cap, "%s", unit->log_mem);
    }
  } else if (streq(argv[0], "show")) {
    struct unit *unit = unit_by_name(argc > 1 ? argv[1] : "");
    if (unit != NULL) {
      append_response(out, cap, "Name=%s\nType=%s\nState=%s\nPID=%d\nCapability=%s\n", unit->name,
                      type_name(unit->type), state_name(unit->state), (int)unit->pid, unit->capability);
    }
  } else if (streq(argv[0], "cat")) {
    cat_unit(out, cap, argc > 1 ? argv[1] : "");
  } else if (streq(argv[0], "is-active")) {
    struct unit *unit = unit_by_name(argc > 1 ? argv[1] : "");
    append_response(out, cap, "%s\n", unit != NULL && unit->state == STATE_ACTIVE ? "active" : "inactive");
  } else if (streq(argv[0], "is-failed")) {
    struct unit *unit = unit_by_name(argc > 1 ? argv[1] : "");
    append_response(out, cap, "%s\n", unit != NULL && unit->state == STATE_FAILED ? "failed" : "ok");
  } else if (streq(argv[0], "enable") || streq(argv[0], "disable")) {
    append_response(out, cap, "%s %s: install links recorded by image manifest\n", argv[0], argc > 1 ? argv[1] : "");
  } else if (streq(argv[0], "isolate")) {
    char err[160] = {0};
    (void)start_unit_name(argc > 1 ? argv[1] : "multi-user.target", err, sizeof(err));
    append_response(out, cap, "isolated %s\n", argc > 1 ? argv[1] : "multi-user.target");
  } else if (streq(argv[0], "poweroff") || streq(argv[0], "reboot")) {
    stop_all_reverse(out, cap);
    write(STDOUT_FILENO, out, strlen(out));
    (void)syscall(SYS_SPORE_SHUTDOWN);
  } else {
    append_response(out, cap, "%s: unknown command\n", argv[0]);
  }
}

static void handle_control_client(void) {
  int fd = accept(control_fd, NULL, NULL);
  if (fd < 0) { return; }
  char cmd[256];
  size_t len = 0;
  while (len + 1 < sizeof(cmd)) {
    char c;
    ssize_t n = read(fd, &c, 1);
    if (n <= 0 || c == '\n') { break; }
    cmd[len++] = c;
  }
  cmd[len] = '\0';
  char out[4096] = {0};
  reap_children();
  handle_command(cmd, out, sizeof(out));
  if (out[0] != '\0') { (void)write(fd, out, strlen(out)); }
  close(fd);
}

static void setup_control_socket(void) {
  ensure_dir("/run/mycelium");
  (void)unlink("/run/mycelium.sock");
  control_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (control_fd < 0) {
    perror("mycelium: socket");
    return;
  }
  struct sockaddr_un sa;
  memset(&sa, 0, sizeof(sa));
  sa.sun_family = AF_UNIX;
  snprintf(sa.sun_path, sizeof(sa.sun_path), "/run/mycelium.sock");
  if (bind(control_fd, (struct sockaddr *)&sa, sizeof(sa)) != 0 || listen(control_fd, 8) != 0) {
    perror("mycelium: control");
    close(control_fd);
    control_fd = -1;
  }
}

static void print_motd(void) {
  int fd = open("/etc/motd", O_RDONLY);
  if (fd < 0) { return; }
  char buf[128];
  for (;;) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n <= 0) { break; }
    (void)write(STDOUT_FILENO, buf, (size_t)n);
  }
  close(fd);
}

int main(void) {
  setenv("PATH", "/bin:/sbin:/usr/bin:/usr/local/bin:.", 0);
  setenv("HOME", "/root", 0);
  setenv("USER", "root", 0);
  setenv("LOGNAME", "root", 0);
  setenv("SHELL", "/bin/msh", 0);
  setenv("TERM", "xterm-256color", 0);
  
  ensure_dir("/run/mycelium");
  ensure_dir("/var/log/mycelium");
  ensure_dir("/var/lib/mycelium");
  load_units();
  setup_control_socket();

  char err[160] = {0};
  (void)start_unit_name("multi-user.target", err, sizeof(err));
  puts("spore: mycelium starting, reached multi-user.target");
  print_motd();
  fflush(stdout);
  defer_console_start = false;
  (void)start_unit_name("console.service", err, sizeof(err));

  for (;;) {
    reap_children();
    check_timers();
    check_watchdogs();

    struct pollfd fds[UNIT_CAP + 1];
    struct unit *journal_units[UNIT_CAP];
    nfds_t nfds = 0;
    if (control_fd >= 0) { fds[nfds++] = (struct pollfd){.fd = control_fd, .events = POLLIN}; }
    for (size_t i = 0; i < UNIT_CAP && nfds < UNIT_CAP + 1; ++i) {
      if (units[i].used && units[i].journal_fd >= 0) {
        journal_units[nfds] = &units[i];
        fds[nfds++] = (struct pollfd){.fd = units[i].journal_fd, .events = POLLIN | POLLHUP};
      }
    }
    int rc = poll(fds, nfds, next_poll_timeout_ms());
    if (rc > 0) {
      nfds_t start = 0;
      if (control_fd >= 0) {
        if ((fds[0].revents & POLLIN) != 0) { handle_control_client(); }
        start = 1;
      }
      for (nfds_t i = start; i < nfds; ++i) {
        if ((fds[i].revents & (POLLIN | POLLHUP)) != 0) { read_journal_fd(journal_units[i]); }
      }
    }
  }
}
