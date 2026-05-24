userland_df = custom_target('userland-df',
  input: files('main.c') + userland_libspore,
  output: 'df',
  command: userland_cc + ['@INPUT@', '-o', '@OUTPUT@'])
userland_image_targets += userland_df
