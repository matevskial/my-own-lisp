project('my-own-lisp', 'c',
        version : '0.0.1',
        default_options : ['warning_level=3', 'c_std=c23'])

c = meson.get_compiler('c')

subdir('tui')

sources = [files('main.c')] + tui_sources
includes = [tui_inc]

dependencies_for_target_unix_mac = []

# I am choosing this approach since its most explicit
editlineDep = declare_dependency(link_args : ['-ledit'])

# this works
#editlineDep = c.find_library('edit')

# this does not work
#editlineDep = dependency('edit', method : 'system')

if editlineDep.found()
        dependencies_for_target_unix_mac += [editlineDep]
endif

my_own_lisp_unix_mac = executable(
        'my_own_lisp_unix_mac',
        sources,
        include_directories : includes,
        dependencies : dependencies_for_target_unix_mac,
        install : true)

test('test_unix_mac', my_own_lisp_unix_mac)
