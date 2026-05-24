userland_tee = custom_target('userland-tee',
  input: files('main.c') + userland_libspore,
  output: 'tee',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_tee
