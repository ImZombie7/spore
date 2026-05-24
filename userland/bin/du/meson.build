userland_du = custom_target('userland-du',
  input: files('main.c') + userland_libspore,
  output: 'du',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_du
