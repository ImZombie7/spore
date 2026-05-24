userland_tar = custom_target('userland-tar',
  input: files('main.c') + userland_libspore,
  output: 'tar',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_tar
