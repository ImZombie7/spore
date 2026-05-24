userland_date = custom_target('userland-date',
  input: files('main.c') + userland_libspore,
  output: 'date',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_date
