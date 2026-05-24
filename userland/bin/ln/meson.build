userland_ln = custom_target('userland-ln',
  input: files('main.c') + userland_libspore,
  output: 'ln',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_ln
