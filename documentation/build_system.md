# Build System
Each of the `build.bat` and `build.sh ` files have a bit of extra documentation
in the header of the file. Please read that for more info.

# Linux Version
All of the Linux build.sh arguments have been set up in a way where you can
specify it as command line arguments or environment variables. This is by pure
coincidence, but it does work, and it is nice. 

This version has as close to feature parity with the build.bat as possible, even
though a lot of the commands are semi-nonsense unless using something niche like
mingw, but its being left that way as to not make any assumptions for the time
being to allow any major changes to be left to somebody better informed in the
future. One notable change is the removal of the logo embed because
unfortunately Linux does not have an equivilent of that, however in the future
this can be turned into a `.desktop` file.

## TODO
- [ ] Make a `.desktop` file on build

## Compiler Flags
* -lpthread | POSIX threading library
* -ldl | dynamic linking library
* -lrt | realtime time library for clock functionality
* -lfreetype | freetype font provider library
* -latomic | GLIBC atomic intrinsics
* -m | Standard Math Library
* -Wno=parenthesis | Checks surprising gotchas with operator precedence, seems
  mostly like basic noob trap mistakes, think we can ignore it here.
* -Werror=atomic-memory-ordering | This is probably an actual major bug if it appears.
* -std=c11 | pin version to some oldish C standard to try to increase version support
* -D_DEFAULT_SOURCE=1 provides missing definitions like 'futimes' that vanish when '-std=c11' is used
  It's not entirely clear why this works this way but _GNU_SOURCE=1 was providing it previously

## Linker Flags
* `-Wl,-z,notext` this linker flag was given to allow metagen to relocate data in the read only segment, it gave the option between that and "-fPIC". This is the exact compiler output.
```
ld.lld: error: can't create dynamic relocation R_X86_64_64 against local symbol in readonly segment; recompile object files with -fPIC or pass '-Wl,-z,notext' to allow text relocations in the output
>>> defined in /tmp/metagen_main-705025.o
>>> referenced by metagen_main.c
>>>               /tmp/metagen_main-705025.o:(mg_str_expr_op_symbol_string_table)
```
