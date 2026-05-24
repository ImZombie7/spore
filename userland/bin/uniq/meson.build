userland_uniq = custom_target('userland-uniq',
  input: files('main.c') + userland_libspore,
  output: 'uniq',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_uniq
