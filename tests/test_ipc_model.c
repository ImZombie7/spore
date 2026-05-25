#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
  MODEL_CAP = 4,
  MODEL_EAGAIN = -11,
  MODEL_EPIPE = -32,
};

struct model_pipe {
  bool used;
  bool had_writer;
  uint16_t readers;
  uint16_t writers;
  uint8_t head;
  uint8_t len;
  char data[MODEL_CAP];
};

struct model_unix_stream {
  struct model_pipe c2s;
  struct model_pipe s2c;
};

static void pipe_init(struct model_pipe *pipe, bool fifo) {
  memset(pipe, 0, sizeof(*pipe));
  pipe->used = true;
  pipe->had_writer = !fifo;
}

static int pipe_write(struct model_pipe *pipe, const char *src, int len) {
  if (pipe->readers == 0) { return MODEL_EPIPE; }
  int done = 0;
  while (done < len && pipe->len < MODEL_CAP) {
    uint8_t tail = (uint8_t)((pipe->head + pipe->len) % MODEL_CAP);
    pipe->data[tail] = src[done++];
    ++pipe->len;
  }
  return done == 0 ? MODEL_EAGAIN : done;
}

static int pipe_read(struct model_pipe *pipe, char *dst, int len) {
  int done = 0;
  while (done < len && pipe->len != 0) {
    dst[done++] = pipe->data[pipe->head];
    pipe->head = (uint8_t)((pipe->head + 1u) % MODEL_CAP);
    --pipe->len;
  }
  if (done != 0) { return done; }
  return pipe->writers == 0 && pipe->had_writer ? 0 : MODEL_EAGAIN;
}

static void pipe_add_reader(struct model_pipe *pipe) {
  ++pipe->readers;
}

static void pipe_add_writer(struct model_pipe *pipe) {
  ++pipe->writers;
  pipe->had_writer = true;
}

static void pipe_drop_reader(struct model_pipe *pipe) {
  assert(pipe->readers > 0);
  --pipe->readers;
  pipe->used = pipe->readers != 0 || pipe->writers != 0;
}

static void pipe_drop_writer(struct model_pipe *pipe) {
  assert(pipe->writers > 0);
  --pipe->writers;
  pipe->used = pipe->readers != 0 || pipe->writers != 0;
}

static void test_anonymous_pipe(void) {
  struct model_pipe pipe;
  char out[8] = {0};
  pipe_init(&pipe, false);
  pipe_add_reader(&pipe);
  pipe_add_writer(&pipe);

  assert(pipe_write(&pipe, "abcdef", 6) == 4);
  assert(pipe_write(&pipe, "z", 1) == MODEL_EAGAIN);
  assert(pipe_read(&pipe, out, 2) == 2);
  assert(memcmp(out, "ab", 2) == 0);
  assert(pipe_write(&pipe, "xy", 2) == 2);
  memset(out, 0, sizeof(out));
  assert(pipe_read(&pipe, out, 8) == 4);
  assert(memcmp(out, "cdxy", 4) == 0);
  assert(pipe_read(&pipe, out, 1) == MODEL_EAGAIN);

  pipe_drop_writer(&pipe);
  assert(pipe_read(&pipe, out, 1) == 0);
  pipe_drop_reader(&pipe);
  assert(!pipe.used);
}

static void test_fifo_eof_and_epipe(void) {
  struct model_pipe pipe;
  char out[2] = {0};
  pipe_init(&pipe, true);
  pipe_add_reader(&pipe);
  assert(pipe_read(&pipe, out, 1) == MODEL_EAGAIN);
  pipe_add_writer(&pipe);
  assert(pipe_write(&pipe, "q", 1) == 1);
  pipe_drop_writer(&pipe);
  assert(pipe_read(&pipe, out, 1) == 1);
  assert(out[0] == 'q');
  assert(pipe_read(&pipe, out, 1) == 0);
  pipe_drop_reader(&pipe);

  pipe_init(&pipe, false);
  pipe_add_reader(&pipe);
  pipe_add_writer(&pipe);
  pipe_drop_reader(&pipe);
  assert(pipe_write(&pipe, "x", 1) == MODEL_EPIPE);
  pipe_drop_writer(&pipe);
}

static void test_unix_stream_pair(void) {
  struct model_unix_stream conn;
  char out[8] = {0};
  pipe_init(&conn.c2s, false);
  pipe_init(&conn.s2c, false);
  pipe_add_writer(&conn.c2s);
  pipe_add_reader(&conn.c2s);
  pipe_add_writer(&conn.s2c);
  pipe_add_reader(&conn.s2c);

  assert(pipe_write(&conn.c2s, "ping", 4) == 4);
  assert(pipe_read(&conn.c2s, out, sizeof(out)) == 4);
  assert(memcmp(out, "ping", 4) == 0);
  assert(pipe_write(&conn.s2c, "pong", 4) == 4);
  memset(out, 0, sizeof(out));
  assert(pipe_read(&conn.s2c, out, sizeof(out)) == 4);
  assert(memcmp(out, "pong", 4) == 0);

  pipe_drop_writer(&conn.c2s);
  assert(pipe_read(&conn.c2s, out, 1) == 0);
  pipe_drop_reader(&conn.s2c);
  assert(pipe_write(&conn.s2c, "!", 1) == MODEL_EPIPE);
}

int main(void) {
  test_anonymous_pipe();
  test_fifo_eof_and_epipe();
  test_unix_stream_pair();
  return 0;
}
