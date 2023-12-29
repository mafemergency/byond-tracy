## byond-tracy
byond-tracy glues together a byond server with the tracy profiler allowing you to analyze and visualize proc calls

## supported byond versions
| windows  | linux    |
| -------- | -------- |
| 515.1623 | 515.1623 |
| 515.1622 | 515.1622 |
| 515.1621 | 515.1621 |
| 515.1620 | 515.1620 |
| 515.1619 | 515.1619 |
| 515.1618 | 515.1618 |
| 515.1617 | 515.1617 |
| 515.1616 | 515.1616 |
| 515.1615 | 515.1615 |
| 515.1614 | 515.1614 |
| 515.1613 | 515.1613 |
| 515.1612 |          |
| 515.1611 | 515.1611 |
| 515.1610 | 515.1610 |
| 515.1609 | 515.1609 |
| 515.1608 | 515.1608 |
| 515.1607 | 515.1607 |
| 515.1606 | 515.1606 |
| 515.1605 | 515.1605 |
| 515.1604 | 515.1604 |
| 515.1603 | 515.1603 |
| 515.1602 | 515.1602 |
| 515.1601 | 515.1601 |
| 515.1600 | 515.1600 |
| 515.1599 | 515.1599 |
| 515.1598 | 515.1598 |
| 515.1597 | 515.1597 |
| 515.1596 | 515.1596 |
| 515.1595 | 515.1595 |
| 515.1594 | 515.1594 |
| 515.1593 | 515.1593 |
| 515.1592 | 515.1592 |
| 515.1591 | 515.1591 |
| 515.1590 | 515.1590 |
| 514.*    | 514.*    |

## supported tracy versions
`0.8.1` `0.8.2` `0.9.0` `0.9.1` `0.10.0`

## usage
simply call `init` from `prof.dll` to begin collecting profile data and connect using [tracy-server](https://github.com/wolfpld/tracy/releases) `Tracy.exe`
```dm
/proc/prof_init()
	var/lib

	switch(world.system_type)
		if(MS_WINDOWS) lib = "prof.dll"
		if(UNIX) lib = "libprof.so"
		else CRASH("unsupported platform")

	var/wait_for_profiler_attach = TRUE
	var/init = wait_for_profiler_attach ? call_ext(lib, "init")() : call_ext(lib, "init")("block")
	if("0" != init) CRASH("[lib] init error: [init]")

/world/New()
	prof_init()
	. = ..()
```

## env vars
set these env vars before launching dreamdaemon to control which node and service to bind
```console
UTRACY_BIND_ADDRESS
```

```console
UTRACY_BIND_PORT
```

## building
no build system included, simply invoke your preferred c11 compiler.
examples:
```console
cl.exe /nologo /std:c11 /O2 /LD /DNDEBUG prof.c ws2_32.lib /Fe:prof.dll
```

```console
clang.exe -std=c11 -m32 -shared -Ofast3 -DNDEBUG -fuse-ld=lld-link prof.c -lws2_32 -o prof.dll
```

```console
gcc -std=c11 -m32 -shared -fPIC -Ofast -s -DNDEBUG prof.c -pthread -o libprof.so
```

## remarks
byond-tracy is in its infancy and is not production ready for live servers.
