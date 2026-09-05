.PHONY: configure build run

configure:
	cmake -S . -B build

build:
	cmake --build build

run:
	@build/linux-camera

run_ff:
	@build/linux-camera-ffmpeg
