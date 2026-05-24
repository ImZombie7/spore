userland_yes = custom_target('userland-yes',
  input: files('main.c') + userland_libspore,
  output: 'yes',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_yes
