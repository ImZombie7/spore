userland_basename = custom_target('userland-basename',
  input: files('main.c') + userland_libspore,
  output: 'basename',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_basename
