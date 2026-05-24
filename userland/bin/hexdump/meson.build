userland_hexdump = custom_target('userland-hexdump',
  input: files('main.c') + userland_libspore,
  output: 'hexdump',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_hexdump
