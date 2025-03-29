project('_meson_module', 'c')

py = import('python').find_installation(pure: false)
python_bin = find_program('python3', 'python')
pyawaitable_include = run_command(python_bin, '-m', 'pyawaitable', '--include', check: true).stdout().strip()


py.extension_module(
    '_meson_module',
    'module.c',
    install: true,
    include_directories: [pyawaitable_include],
)