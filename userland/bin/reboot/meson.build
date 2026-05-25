userland_reboot = custom_target('userland-reboot',
  input: files('main.c') + userland_libspore,
  output: 'reboot',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_reboot
