linux_make:
	gcc source/main.c -o main
windows_msys2_make:
	gcc source/main.c -o main.exe
windows_vs_nmake:
	cl source/main.c