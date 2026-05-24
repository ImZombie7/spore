userland_id = custom_target('userland-id',
  input: files('main.c') + userland_libspore,
  output: 'id',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_id
