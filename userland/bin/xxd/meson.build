userland_xxd = custom_target('userland-xxd',
  input: files('main.c') + userland_libspore,
  output: 'xxd',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_xxd
