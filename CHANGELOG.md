# Change Log v9.22.0-alpha

## Linker

- Changed symbol resolution in libaries to match MSVC behavior.
- Optimized image building step to reduce memory usage.
- Linker memory maps all input files by default to lower memory usage. (`/RAD_MEMORY_MAP_FILES`)
- If debug info is available, linker uses it to show file and line number for unresolved relocations.
- Improved base relocation build performance for large images, cutting build time by 70%.
- Added stubs for `/Brepro`, `/D2`, and /ErrorReport to improve compatability with existing response files
- Implemented section garbage collection (`/OPT:REF`)
- Fixed bug where thread local variables pointed to incorrect types.
- Changed rules for weak and undefined symbols, now weak symbol is not allowed to replace an undefined symbol.
- Linker no longer creates thunks for imports that don't require them.


