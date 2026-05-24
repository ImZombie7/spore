userland_top = custom_target('userland-top',
  input: files('main.c') + userland_libspore,
  output: 'top',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_top
