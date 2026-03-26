CC := gcc
CFLAGS := -O2 -Wall

.PHONY: all clean package

all:
	@mkdir -p build
	cmake -S . -B build -DWITH_PKGTREE=ON
	cmake --build build --config Release

clean:
	-rm -rf build
	-rm -rf .tree
	-rm -f tree

package:
	@mkdir -p build
	cmake -S . -B build -DWITH_PKGTREE=ON
	cmake --build build --config Release --target package
