project('my-own-lisp', 'c',
        version : '0.0.1',
        default_options : ['warning_level=3', 'c_std=c23'])

sources = [files('main.c')]
includes = []

my_own_lisp = executable(
        'my_own_lisp',
        sources,
        include_directories : includes,
        install : true)

test('test', my_own_lisp)
