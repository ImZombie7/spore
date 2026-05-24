userland_cp = custom_target('userland-cp',
  input: files('main.c') + userland_libspore,
  output: 'cp',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_cp
