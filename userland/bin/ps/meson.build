userland_ps = custom_target('userland-ps',
  input: files('main.c') + userland_libspore,
  output: 'ps',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_ps
