## byond-tracy
byond-tracy glues together a byond server with the tracy profiler allowing you to analyze and visualize proc calls

## supported byond versions
| windows  | linux    |
| -------- | -------- |
| 514.1584 |          |
| 514.1583 |          |

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
where clang
> C:\Program Files\LLVM\bin\clang.exe
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

building with msvc 2019 and msbuild (multi config):
```console
vcvarsall.bat x86
where cl
> C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.29.30133\bin\Hostx86\x86\cl.exe
cmake -Bbuild -G"Visual Studio 16 2019" -AWin32
cmake --build build --config=Release
```

## remarks
byond-tracy is in its infancy and is not production ready for live servers.
at present there is no mechanism to disable the profiler, selectively profile specific procs, no cleanup on server shutdown/reboot, etc. byond-tracy will buffer profile data in memory until a connection is made with tracy-server to consume the events meaning your server will ecounter an out-of-memory situation should you neglect to connect.
