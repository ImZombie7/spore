userland_kill = custom_target('userland-kill',
  input: files('main.c') + userland_libspore,
  output: 'kill',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_kill
