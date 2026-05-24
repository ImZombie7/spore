userland_cksum = custom_target('userland-cksum',
  input: files('main.c') + userland_libspore,
  output: 'cksum',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_cksum
