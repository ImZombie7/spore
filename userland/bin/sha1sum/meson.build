userland_sha1sum = custom_target('userland-sha1sum',
  input: files('main.c') + userland_libspore,
  output: 'sha1sum',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_sha1sum
