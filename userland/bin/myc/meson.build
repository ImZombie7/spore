userland_myc = custom_target('userland-myc',
  input: files('main.c') + userland_libspore,
  output: 'myc',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_myc
