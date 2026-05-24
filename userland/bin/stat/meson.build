userland_stat = custom_target('userland-stat',
  input: files('main.c') + userland_libspore,
  output: 'stat',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_stat
