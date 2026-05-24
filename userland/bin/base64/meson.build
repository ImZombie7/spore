userland_base64 = custom_target('userland-base64',
  input: files('main.c') + userland_libspore,
  output: 'base64',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_base64
