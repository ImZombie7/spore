userland_uptime = custom_target('userland-uptime',
  input: files('main.c') + userland_libspore,
  output: 'uptime',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_uptime
