cmake_minimum_required(VERSION 3.15...3.30)
project(${SKBUILD_PROJECT_NAME} LANGUAGES C)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

Python_add_library(_sbc_module MODULE src/module.c WITH_SOABI)
target_include_directories(_sbc_module PRIVATE $ENV{PYAWAITABLE_INCLUDE})
install(TARGETS _sbc_module DESTINATION ${SKBUILD_PROJECT_NAME})
