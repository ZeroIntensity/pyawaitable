import optparse
from typing import Literal
from pyawaitable import __version__, include as get_include

DEFAULT_MESSAGE = f"""PyAwaitable {__version__}
Documentation: https://pyawaitable.zintensity.dev
Source: https://github.com/ZeroIntensity/pyawaitable"""

def option_main(include: bool, version: bool) -> None:
    if include:
        print(get_include())
    if version:
        print(__version__)

    if not include and not version:
        print(DEFAULT_MESSAGE)


def option_as_bool(value: None | Literal[True]) -> bool:
    if value is None:
        return False
    return True


def main():
    parser = optparse.OptionParser()
    parser.add_option("--include", action="store_true")
    parser.add_option("--version", action="store_true")
    opts, _ = parser.parse_args()
    option_main(option_as_bool(opts.include), option_as_bool(opts.version),)

if __name__ == "__main__":
    main()
