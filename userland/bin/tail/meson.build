userland_tail = custom_target('userland-tail',
  input: files('main.c') + userland_libspore,
  output: 'tail',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_tail
