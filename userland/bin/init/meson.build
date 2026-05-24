userland_init = custom_target('userland-init',
  input: files('main.c') + userland_libspore,
  output: 'init',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_init
