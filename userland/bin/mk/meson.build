userland_mk = custom_target('userland-mk',
  input: files('main.c') + userland_libspore,
  output: 'mk',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_mk
