userland_edit = custom_target('userland-edit',
  input: files('main.c') + userland_libspore,
  output: 'edit',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_edit
