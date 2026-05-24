userland_chmod = custom_target('userland-chmod',
  input: files('main.c') + userland_libspore,
  output: 'chmod',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_chmod
