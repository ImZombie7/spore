userland_sum = custom_target('userland-sum',
  input: files('main.c') + userland_libspore,
  output: 'sum',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_sum
