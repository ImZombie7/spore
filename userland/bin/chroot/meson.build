userland_chroot = custom_target('userland-chroot',
  input: files('main.c') + userland_libspore,
  output: 'chroot',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_chroot
