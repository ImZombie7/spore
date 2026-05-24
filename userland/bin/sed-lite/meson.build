userland_sed_lite = custom_target('userland-sed-lite',
  input: files('main.c') + userland_libspore,
  output: 'sed-lite',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_sed_lite
