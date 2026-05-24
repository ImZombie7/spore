userland_printf = custom_target('userland-printf',
  input: files('main.c') + userland_libspore,
  output: 'printf',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_printf
