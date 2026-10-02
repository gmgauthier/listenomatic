project(
  'listenomatic',
  'cpp',
  version: '1.0.13',
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
  'src/resume.cpp',
  'src/live_log.cpp',
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

listenomatic_tests = executable(
  'listenomatic-tests',
  'tests/test_rss.cpp',
  'src/rss.cpp',
  include_directories: inc,
  dependencies: [libxml],
)
test('rss', listenomatic_tests)

resume_tests = executable(
  'listenomatic-resume-tests',
  'tests/test_resume.cpp',
  'src/resume.cpp',
  'src/settings.cpp',
  'src/station.cpp',
  'src/paths.cpp',
  include_directories: inc,
  dependencies: [gtkmm],
)
test('resume', resume_tests)

show_switch_tests = executable(
  'listenomatic-show-switch-tests',
  'tests/test_show_switch.cpp',
  'src/resume.cpp',
  'src/settings.cpp',
  'src/station.cpp',
  'src/paths.cpp',
  include_directories: inc,
  dependencies: [gtkmm],
)
test('show-switch', show_switch_tests)

feed_load_tests = executable(
  'listenomatic-feed-load-tests',
  'tests/test_feed_load.cpp',
  include_directories: inc,
)
test('feed-load', feed_load_tests)

resume_seek_tests = executable(
  'listenomatic-resume-seek-tests',
  'tests/test_resume_seek.cpp',
  include_directories: inc,
)
test('resume-seek', resume_seek_tests)

show_result_tests = executable(
  'listenomatic-show-result-tests',
  'tests/test_show_result.cpp',
  include_directories: inc,
)
test('show-result', show_result_tests)

settings_tests = executable(
  'listenomatic-settings-tests',
  'tests/test_settings.cpp',
  'src/settings.cpp',
  'src/station.cpp',
  'src/paths.cpp',
  include_directories: inc,
  dependencies: [gtkmm],
)
test('settings', settings_tests)

station_tests = executable(
  'listenomatic-station-tests',
  'tests/test_station.cpp',
  'src/station.cpp',
  include_directories: inc,
)
test('station', station_tests)

live_log_tests = executable(
  'listenomatic-live-log-tests',
  'tests/test_live_log.cpp',
  'src/live_log.cpp',
  include_directories: inc,
)
test('live_log', live_log_tests)
