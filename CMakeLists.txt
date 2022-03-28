cmake_minimum_required(VERSION 3.17...3.22)

project(gco_ext
        LANGUAGES CXX
        VERSION 0.1
        DESCRIPTION "GCO")

if (NOT SKBUILD)
  message(FATAL_ERROR "This CMake file should be executed via scikit-build. "
                      "Please run\n$ pip install .")
endif()


# Scikit-Build does not add your site-packages to the search path
# automatically, so we need to add it _or_ the pybind11 specific directory
# here.
execute_process(
COMMAND "${PYTHON_EXECUTABLE}" -c
        "import pybind11; print(pybind11.get_cmake_dir())"
OUTPUT_VARIABLE _tmp_dir
OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ECHO STDOUT)
list(APPEND CMAKE_PREFIX_PATH "${_tmp_dir}")

find_package(pybind11 CONFIG REQUIRED)

add_subdirectory(extern/gco)

pybind11_add_module(
  gco_ext
  MODULE
  src/gco_ext.cpp
)

# Link against GCO
target_link_libraries(gco_ext PRIVATE gco)

# Platform specific options
if(UNIX AND NOT APPLE)
    # Ensure we can find the gco target at runtime
    set_target_properties(gco_ext PROPERTIES
        INSTALL_RPATH "$ORIGIN/${CMAKE_INSTALL_LIBDIR}")
elseif(APPLE)
    # Ensure we can find the gco target at runtime
    set_target_properties(gco_ext PROPERTIES
        INSTALL_RPATH "@loader_path/${CMAKE_INSTALL_LIBDIR}")
endif()

install(TARGETS gco_ext DESTINATION .)
