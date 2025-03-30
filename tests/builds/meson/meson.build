project('_meson_module', 'c')

py = import('python').find_installation(pure: false)
pyawaitable_include = run_command('pyawaitable', '--include', check: true).stdout().strip()


py.extension_module(
    '_meson_module',
    'module.c',
    install: true,
    include_directories: [pyawaitable_include],
)