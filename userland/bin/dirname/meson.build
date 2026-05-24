userland_dirname = custom_target('userland-dirname',
  input: files('main.c') + userland_libspore,
  output: 'dirname',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_dirname
