userland_mv = custom_target('userland-mv',
  input: files('main.c') + userland_libspore,
  output: 'mv',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_mv
