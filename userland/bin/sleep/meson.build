userland_sleep = custom_target('userland-sleep',
  input: files('main.c') + userland_libspore,
  output: 'sleep',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_sleep
