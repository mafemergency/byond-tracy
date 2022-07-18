## byond-tracy
byond-tracy glues together a byond server with the tracy profiler allowing you to analyze and visualize proc calls

## supported byond versions
| windows  | linux    |
| -------- | -------- |
| 514.*    |          |

## supported tracy versions
0.8.1
0.8.2

## usage
simply call `init` from `prof.dll` to begin collecting profile data and connect using [tracy-server](https://github.com/wolfpld/tracy/releases) `Tracy.exe`
```c
/world/New()
	/* assuming prof.dll is in the same directory as the dmb */
	world.log << call("prof.dll", "init")()
	. = ..()
```
or
```c
/* this will be called before /world/New, allowing you to profile /world/New as well */
/datum/early_init/New()
	world.log << call("prof.dll", "init")()
	. = ..()

/var/datum/early_init/glob_early_init = new
```

## building
it is important to remember that dreamdaemon is a 32-bit program, so you must cross-compile when on a 64-bit host

building with clang and ninja (single config):
```console
> clang -v
clang version 14.0.0
> cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release
> cmake --build build
```

building with msvc and msbuild (multi config):
```console
> vcvarsall.bat x86
> cl
Microsoft (R) C/C++ Optimizing Compiler Version 19.29.30140 for x86
> cmake -Bbuild -G"Visual Studio 16 2019" -AWin32
> cmake --build build --config=Release
```

building with gcc and make (single config):
```console
> gcc -v
Target: x86_64-w64-mingw32
gcc version 10.3.0 (tdm64-1)
> cmake -Bbuild -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
> cmake --build build
```

building without cmake:
```console
> ninja -f build\msvc.ninja
> ninja -f build\llvm.ninja
> ninja -f build\gcc.ninja
```

## remarks
byond-tracy is in its infancy and is not production ready for live servers.
