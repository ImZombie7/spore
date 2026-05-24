userland_uname = custom_target('userland-uname',
  input: files('main.c') + userland_libspore,
  output: 'uname',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_uname
