userland_sort = custom_target('userland-sort',
  input: files('main.c') + userland_libspore,
  output: 'sort',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_sort
