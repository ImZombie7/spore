BUILD_DIR ?= build
MESON ?= meson
QEMU ?= qemu-system-aarch64

LLVM_PREFIX := $(shell brew --prefix llvm 2>/dev/null)
ifneq ($(LLVM_PREFIX),)
export PATH := $(LLVM_PREFIX)/bin:$(PATH)
endif

EDK2_CODE ?= $(shell \
	for dir in $$($(QEMU) -L help 2>/dev/null); do \
		for file in "$$dir/edk2-aarch64-code.fd" "$${dir%-firmware}/qemu/edk2-aarch64-code.fd"; do \
			if [ -f "$$file" ]; then printf "%s" "$$file"; exit 0; fi; \
		done; \
	done; \
	if [ -f /opt/homebrew/opt/qemu/share/qemu/edk2-aarch64-code.fd ]; then \
		printf "%s" /opt/homebrew/opt/qemu/share/qemu/edk2-aarch64-code.fd; \
	fi)

.PHONY: setup build test run run-tests clean

setup:
	@test -d "$(BUILD_DIR)" || $(MESON) setup "$(BUILD_DIR)" --cross-file cross/aarch64-elf.ini

build: setup
	$(MESON) compile -C "$(BUILD_DIR)"

test: build
	$(MESON) test -C "$(BUILD_DIR)"

run: build
	@test -n "$(EDK2_CODE)" || (echo "could not find QEMU EDK2 AArch64 firmware" >&2; exit 1)
	$(QEMU) -M virt,gic-version=3 -accel hvf -cpu host -m 512M \
		-boot order=d,menu=off,strict=on \
		-drive if=pflash,format=raw,readonly=on,file="$(EDK2_CODE)" \
		-cdrom "$(BUILD_DIR)/image.iso" \
		-serial stdio -display none

run-tests: setup
	$(MESON) compile -C "$(BUILD_DIR)" run-tests

clean:
	rm -rf "$(BUILD_DIR)"
