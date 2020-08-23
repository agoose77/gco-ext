import os
import shutil
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.source_dir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        ext_dir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        # required for auto-detection of auxiliary "native" libs
        if not ext_dir.endswith(os.path.sep):
            ext_dir += os.path.sep

        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + ext_dir]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        cmake_args += [f'-DCMAKE_BUILD_TYPE={cfg}']
        build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = f'{env.get("CXXFLAGS", "")} -DVERSION_INFO=\\"{self.distribution.get_version()}\\"'
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.source_dir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

        # Copy includes
        shutil.copytree(os.path.join(ext.source_dir, 'include'), os.path.join(ext_dir, 'include'))


setup(
    name='gco',
    version='0.0.1',
    author='Angus Hollands',
    author_email='goosey15@gmail.com',
    description='GCO library with Python bindings',
    long_description='',
    ext_modules=[CMakeExtension('gco.lib', 'src')],
    packages=['gco'],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
