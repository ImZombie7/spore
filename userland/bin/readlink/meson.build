userland_readlink = custom_target('userland-readlink',
  input: files('main.c') + userland_libspore,
  output: 'readlink',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_readlink
