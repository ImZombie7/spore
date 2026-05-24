userland_hostid = custom_target('userland-hostid',
  input: files('main.c') + userland_libspore,
  output: 'hostid',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_hostid
