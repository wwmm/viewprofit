project(
	'viewprofit',
	'cpp',
	default_options : ['cpp_std=c++17','buildtype=debugoptimized'],
	version: '0.0.1',
	meson_version: '>= 0.52.1'
)

prefix = get_option('prefix')
bindir = join_paths(prefix, get_option('bindir'))
datadir = join_paths(prefix, get_option('datadir'))
localedir = join_paths(prefix, get_option('localedir'))
config_h_dir = include_directories('.')

# Configuration
conf = configuration_data()

conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('LOCALE_DIR', localedir)
conf.set_quoted('VERSION', meson.project_version())

configure_file(output: 'config.h', configuration: conf)

qt5 = import('qt5')

subdir('data')
subdir('src')

