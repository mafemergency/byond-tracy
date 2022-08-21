## byond-tracy
byond-tracy glues together a byond server with the tracy profiler allowing you to analyze and visualize proc calls

## supported byond versions
| windows  | linux    |
| -------- | -------- |
| 514.*    | 514.*    |

## supported tracy versions
`0.8.1` `0.8.2`

## usage
simply call `init` from `prof.dll` to begin collecting profile data and connect using [tracy-server](https://github.com/wolfpld/tracy/releases) `Tracy.exe`
```c
/proc/prof_init()
	var/lib

	switch(world.system_type)
		if(MS_WINDOWS) lib = "prof.dll"
		if(UNIX) lib = "libprof.so"
		else CRASH("unsupported platform")

	var/init = call(lib, "init")()
	if("0" != init) CRASH("[lib] init error: [init]")

/world/New()
	prof_init()
	. = ..()
```

## env vars
set these env vars before launching dreamdaemon to control which node and service to bind
`UTRACY_BIND_ADDRESS`
`UTRACY_BIND_PORT`

## building
no build system included, simply invoke your preferred c11 compiler.
examples:
`cl.exe /nologo /std:c11 /O2 /LD prof.c ws2_32.lib /Fe:prof.dll`
`clang.exe -std=c11 -m32 -shared -Ofast3 -fuse-ld=lld-link prof.c -lws2_32 -o prof.dll`
`gcc -std=c11 -m32 -shared -fPIC -Ofast -s prof.c -pthread -o libprof.so`

## remarks
byond-tracy is in its infancy and is not production ready for live servers.
