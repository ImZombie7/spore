userland_tty = custom_target('userland-tty',
  input: files('main.c') + userland_libspore,
  output: 'tty',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_tty
