import cppyy
import pathlib

__all__ = ['GCO']

_this_dir = pathlib.Path(__file__).parent

cppyy.add_include_path(str(_this_dir / 'include'))
cppyy.add_library_path(str(_this_dir))

# Include and load library
cppyy.include("GCoptimization.h")
cppyy.load_library("gco")

from cppyy.gbl import GCO