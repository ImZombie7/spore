userland_shutdown = custom_target('userland-shutdown',
  input: files('main.c') + userland_libspore,
  output: 'shutdown',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_shutdown
