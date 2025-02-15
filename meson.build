project('my-own-lisp', 'c',
        version : '0.0.1',
        default_options : ['warning_level=3', 'c_std=c23'])

c = meson.get_compiler('c')

subdir('tui')
subdir('mpc')

root_includes = include_directories('.')
sources = [files('main.c')] + tui_sources
includes = [root_includes, tui_inc]
dependencies_for_target_unix_mac = []
dependencies_for_target_windows = []

# I am choosing this approach since its most explicit
editlineDep = declare_dependency(link_args : ['-ledit'])

# this works
#editlineDep = c.find_library('edit')

# this does not work
#editlineDep = dependency('edit', method : 'system')

linuxMathDep = declare_dependency(link_args : ['-lm'])

mpcDep = declare_dependency(
        include_directories : mpc_inc,
        sources : mpc_sources
)

dependencies_for_target_unix_mac += [editlineDep, linuxMathDep, mpcDep]
dependencies_for_target_windows += [mpcDep]

my_own_lisp_unix_mac = executable(
        'my_own_lisp_unix_mac',
        sources,
        include_directories : includes,
        dependencies : dependencies_for_target_unix_mac,
        install : true)

my_own_lisp_unix_mac_read_line_stdin_with_stdio = executable(
        'my_own_lisp_unix_mac_read_line_stdin_with_stdio',
        sources,
        include_directories : includes,
        dependencies : dependencies_for_target_unix_mac,
        c_args: ['-DREAD_LINE_STDIN_WITH_STDIO'],
        install : true)

my_own_lisp_windows = executable(
        'my_own_lisp_windows',
        sources,
        include_directories : includes,
        dependencies : dependencies_for_target_windows,
        install : true)

test('test_unix_mac', my_own_lisp_unix_mac)
