userland_mkfifo = custom_target('userland-mkfifo',
  input: files('main.c') + userland_libspore,
  output: 'mkfifo',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_mkfifo
