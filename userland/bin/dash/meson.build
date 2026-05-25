userland_dash = custom_target('userland-dash',
  output: 'dash',
  command: [
    files('build.sh'),
    meson.project_source_root(),
    meson.current_build_dir(),
    '@OUTPUT@',
  ])
userland_image_targets += userland_dash
