userland_find = custom_target('userland-find',
  input: files('main.c') + userland_libspore,
  output: 'find',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_find
