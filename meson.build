project(
  'listenomatic',
  'cpp',
  version: '0.0.0',
  license: 'Unlicense',
  meson_version: '>=0.56',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'prefix=/usr',
  ],
)

gtkmm = dependency('gtkmm-3.0')
gst = dependency('gstreamer-1.0')
soup = dependency('libsoup-3.0')
json_glib = dependency('json-glib-1.0')
libxml = dependency('libxml-2.0')

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('APP_ID', 'org.gmgauthier.ListenOMatic')
conf.set_quoted('DATADIR', get_option('prefix') / get_option('datadir') / 'listenomatic')
conf.set_quoted('SOURCE_ROOT', meson.project_source_root())

configure_file(
  output: 'config.hpp',
  configuration: conf,
)

inc = include_directories('.', 'src')

executable(
  'listenomatic',
  'src/main.cpp',
  'src/application.cpp',
  'src/paths.cpp',
  'src/station.cpp',
  'src/settings.cpp',
  'src/player.cpp',
  'src/marquee.cpp',
  'src/fetch.cpp',
  'src/radiobrowser.cpp',
  'src/itunes.cpp',
  'src/rss.cpp',
  'src/add_dialog.cpp',
  'src/catalog_window.cpp',
  'src/main_window.cpp',
  'src/about_dialog.cpp',
  include_directories: inc,
  dependencies: [gtkmm, gst, soup, json_glib, libxml],
  install: true,
  install_dir: get_option('bindir'),
)

install_data(
  'data/listenomatic.desktop',
  install_dir: get_option('datadir') / 'applications',
)

install_data(
  'data/skin/lcos/lcos.css',
  install_dir: get_option('datadir') / 'listenomatic' / 'skin' / 'lcos',
)

install_data(
  'data/samples.ini',
  install_dir: get_option('datadir') / 'listenomatic',
)

install_data(
  'data/podcasts.ini',
  install_dir: get_option('datadir') / 'listenomatic',
)

install_data(
  'brand/icon-tile.svg',
  install_dir: get_option('datadir') / 'listenomatic' / 'brand',
)

install_data(
  'brand/icon-tile.svg',
  rename: 'listenomatic.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
