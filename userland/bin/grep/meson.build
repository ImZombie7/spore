userland_grep = custom_target('userland-grep',
  input: files('main.c') + userland_libspore,
  output: 'grep',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_grep
