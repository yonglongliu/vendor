from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

setup(
ext_modules = cythonize([
    Extension("pytfa", ["pytfa.pyx"],
              libraries=["tfa98xx"])
    ], gdb_debug=True)
)

# extensions =  Extension("pytfa", ["pytfa.pyx"],libraries=["tfa98xx"])
#  
# setup( ext_modules=cythonize(extensions, gdb_debug=True))
