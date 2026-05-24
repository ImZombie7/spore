userland_hostname = custom_target('userland-hostname',
  input: files('main.c') + userland_libspore,
  output: 'hostname',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_hostname
