# Contributing to PyAwaitable

Lucky for you, the internals of PyAwaitable are extremely well documented, since it was originally designed to be part of the CPython API.

Before you get started, it's a good idea to read the following discussions, or at least skim through them:

-   [Adding a C API for coroutines/awaitables](https://discuss.python.org/t/adding-a-c-api-for-coroutines-awaitables/22786)
-   [C API for asynchronous functions](https://discuss.python.org/t/c-api-for-asynchronous-functions/42842)
-   [Revisiting a C API for asynchronous functions](https://discuss.python.org/t/revisiting-a-c-api-for-asynchronous-functions/50792)

Then, for all the details of the underlying implementation, read the [scrapped PEP](https://gist.github.com/ZeroIntensity/8d32e94b243529c7e1c27349e972d926).

## Development Workflow

You'll first want to find an [issue](https://github.com/ZeroIntensity/pyawaitable/issues) that you want to implement. Make sure not to choose an issue that already has someone assigned to it!

Once you've chosen something you would like to work on, be sure to make a comment requesting that the issue be assigned to you. You can start working on the issue before you've been officially assigned to it on GitHub, as long as you made a comment first.

After you're done, make a [pull request](https://github.com/ZeroIntensity/pyawaitable/pulls) merging your code to the master branch. A successful pull request will have all of the following:

-   A link to the issue that it's implementing.
-   New and passing tests.
-   Updated docs and changelog.
-   Code following the style guide, mentioned below.

## Style Guide

PyAwaitable follows [PEP 7](https://peps.python.org/pep-0007/), so if you've written any code in the CPython core, you'll feel right at home writing code for PyAwaitable.

However, don't bother trying to format things yourself! PyAwaitable provides an [uncrustify](https://github.com/uncrustify/uncrustify) configuration file for you.

## Project Setup

If you haven't already, clone the project.

```
$ git clone https://github.com/ZeroIntensity/pyawaitable
$ cd pyawaitable
```

To build PyAwaitable locally, simple run `pip`:

```
$ pip install .
```

## Running Tests

PyAwaitable uses [Hatch](https://hatch.pypa.io), so that will handle everything for you:

```
$ hatch test
```
