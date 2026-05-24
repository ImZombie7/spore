userland_who = custom_target('userland-who',
  input: files('main.c') + userland_libspore,
  output: 'who',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_who
