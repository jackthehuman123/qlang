.PHONY: build test config clean

build:
	cmake --build build -j

test: build
	ctest --test-dir build --output-on-failure

config:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON

clean:
	rm -rf build