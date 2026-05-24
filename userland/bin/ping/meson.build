userland_ping = custom_target('userland-ping',
  input: files('main.c') + userland_libspore,
  output: 'ping',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_ping
