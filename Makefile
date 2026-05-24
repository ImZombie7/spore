BUILD_DIR ?= build
MESON ?= meson
CLANG_FORMAT ?= clang-format
QEMU_RUNNER ?= $(BUILD_DIR)/tools/spore-run
QEMU ?= qemu-system-aarch64

.PHONY: setup build image test-image runner test run run-tests run-shell-check format clean

setup:
	@test -d "$(BUILD_DIR)" || $(MESON) setup "$(BUILD_DIR)" --cross-file cross/aarch64-elf.ini

build: setup
	$(MESON) compile -C "$(BUILD_DIR)"

image: setup
	$(MESON) compile -C "$(BUILD_DIR)" image.iso

test-image: setup
	$(MESON) compile -C "$(BUILD_DIR)" test_image.iso

runner: setup
	$(MESON) compile -C "$(BUILD_DIR)" spore-run

test: build
	$(MESON) test -C "$(BUILD_DIR)"

run: image runner
	$(QEMU_RUNNER) --mode plain --image "$(BUILD_DIR)/image.iso" --qemu "$(QEMU)"

run-tests: test-image runner
	$(QEMU_RUNNER) --mode filter --image "$(BUILD_DIR)/test_image.iso"

run-shell-check: image runner
	$(QEMU_RUNNER) --mode shell --image "$(BUILD_DIR)/image.iso"

format:
	find bootloader kernel tests userland -type f \( -name '*.c' -o -name '*.h' -o -name '*.cc' -o -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 $(CLANG_FORMAT) -i

clean:
	rm -rf "$(BUILD_DIR)"
