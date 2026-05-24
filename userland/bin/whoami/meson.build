userland_whoami = custom_target('userland-whoami',
  input: files('main.c') + userland_libspore,
  output: 'whoami',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_whoami
