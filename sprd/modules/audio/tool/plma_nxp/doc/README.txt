NXP TFA Host SW Build README
============================

See build.sh in the top folder for build script. It is desigend to run on Cygwin with MSBuild.exe for Windows and SConstruct in Linux.
Android.mk file is available for Android integration. 

Documentation
=============

Use Doxygen for generating HTML documentation. The documentation is generated in doc/ folder. Command:
- doxygen : default only the core integration code 
- doxygen Doxyfile-full : for full source code documentation.
