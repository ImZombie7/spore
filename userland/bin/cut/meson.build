userland_cut = custom_target('userland-cut',
  input: files('main.c') + userland_libspore,
  output: 'cut',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_cut
