userland_head = custom_target('userland-head',
  input: files('main.c') + userland_libspore,
  output: 'head',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_head
