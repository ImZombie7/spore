userland_wc = custom_target('userland-wc',
  input: files('main.c') + userland_libspore,
  output: 'wc',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_wc
