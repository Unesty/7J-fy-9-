project('FurryChainsaw', ['c'],
	version: '1',
	default_options: [
		'werror=false',
		'warning_level=3',
		'c_std=c11',
		'default_library=static',
	]
)

# Options

is_release = (get_option('buildtype') == 'release')

debug_checks = not is_release
disable_dlg = is_release
debug_stats = true

###############################################################

#shader_inc = []
#subdir('src/data') # compile shaders

cc = meson.get_compiler('c')

args = cc.get_supported_arguments([
	# silence various msvc warnings, we still wanna compile with werror
	'/wd4065', # switch has no case, just default
	'/wd26812', # prefer 'enum class' over 'enum'. Warning isn't wrong but can't change external code
	'/wd4244', # lossy type conversion (e.g. double -> int)
	'/wd4305', # truncating type conversion (e.g. double -> float)
	'/wd4127', # conditional expression is constant (might be useful but not really for us)
	'/wd4456', # delecaration hides previous local declaration (sorry, too used to rust's beauty)
	'/wd4458', # declaration hides class member, TODO: should be fixed in code, valid warning
	'/wd5105', # needed to fix an issue in winbase.h on a specific windows version (wtf microsoft)
	'/wd4267', # lossy conversion warning, way too strict.
	'/wd4245', # signed/unsigned warning, way too strict.
	'-D_CRT_SECURE_NO_WARNINGS',

	# msvc really slow in debug build without this
	#'-D_ITERATOR_DEBUG_LEVEL=0',

	# for clang
	'-Wno-missing-braces',
	'-Wno-newline-eof', # needed for spvm TODO re-enable and fix!

	# to be compatible with msvc warning level on gcc/clang
	# debatable whether the warnings are actually useful
	# '-Wconversion',
	# '-Wno-float-conversion',

	'-D_POSIX_C_SOURCE=200112L',
])

# defines
args += ['-Wno-error']

add_project_arguments(args, language: 'c')

if get_option('buildtype') == 'debug'
	# put debugging stuff here
endif

# TODO: yeah, this needs some love
with_win32 = build_machine.system() == 'windows'


# deps
dep_threads = dependency('threads', required: false)
dep_swa = dependency('swa',
	fallback: ['swa', 'swa_dep'],
	static: true,
	default_options: [
		'with-gl=enabled',
		'with-vulkan=disabled',
		'link-vulkan=false',
		'with-kms=enabled'
	])

dep_dlg = dependency('dlg',
	static: true,
	fallback: ['dlg', 'dlg_dep'])
if disable_dlg
	warning('disabling dlg')
endif
dep_gl = dependency('gl', required: true)
cmake = import('cmake')
opt_var = cmake.subproject_options()
opt_var.append_compile_args('c','-Wno-error')
subproj_libsoundio = cmake.subproject('libsoundio', options: opt_var)
dep_libsoundio = subproj_libsoundio.dependency('libsoundio_static')

src = files(
	# main sources for everything
	'src/graphics.c',
	'src/ui_logic.c',
)

public_inc = include_directories('include/')
private_inc = include_directories('src/')

inc = [
	public_inc,
	private_inc,
# 	shader_inc,
]

deps = [
	dep_threads,
	dep_dlg,
	dep_swa,
]


if with_win32
# 	src += files('src/win32.c')
# 	args += []
endif

link_args = []
if get_option('b_coverage')
	link_args += ['--coverage']
endif

if build_machine.system() == 'windows'
	# TODO add as install script via meson.add_install_script?
# 	configure_file(
# 		input: 'register_layer.bat.in',
# 		output: 'register_layer.bat',
# 		install: false,
# 		copy: true,
# 	)
endif

subdir('src')


supervisor = build_target('supervisor',
	'src/supervisor.c',
	target_type: 'executable',
	dependencies: [])
