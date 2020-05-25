run: build
	./build/server

run_gdb: build
	gdb ./build/server

build:
	./build.sh

.PHONY: run run_gdb build
