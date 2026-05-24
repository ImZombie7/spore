userland_time = custom_target('userland-time',
  input: files('main.c') + userland_libspore,
  output: 'time',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_time
