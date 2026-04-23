CMAKE ?= cmake
REPO_DIR := /mnt/c/Users/Veronika/Downloads/Domco/tree

.PHONY: all clean linux wsl package

all: linux

linux:
	@mkdir -p build-linux
	$(CMAKE) -S . -B build-linux -DWITH_PKGTREE=ON
	$(CMAKE) --build build-linux --config Release

wsl:
	wsl bash -lc 'cd $(REPO_DIR) && cmake -S . -B build-wsl -DWITH_PKGTREE=ON && cmake --build build-wsl --config Release'

package: linux
	$(CMAKE) --build build-linux --config Release --target package

clean:
	-rm -rf build build-linux build-wsl
	-rm -rf .tree
	-rm -f tree tree.exe
