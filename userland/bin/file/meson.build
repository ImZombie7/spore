userland_file = custom_target('userland-file',
  input: files('main.c') + userland_libspore,
  output: 'file',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_file
