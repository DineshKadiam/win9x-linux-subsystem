ifndef WATCOM
WATCOM = $(error must set WATCOM env)
endif

LINUX ?= linux

CFLAGS = -std=c11 -Wall -Wextra -Werror -Wstrict-prototypes -funsigned-char \
	-Wno-error=unused-parameter -Wno-error=unused-function -Wno-error=unused-variable

wsl9x-include = \
	-I $(LINUX)/include \
	-I $(LINUX)/arch/um/os-Win95/include

vxd-objs = vxd/wsl9x.o vxd/debug.o vxd/loader.o vxd/cvxdwrap.o vxd/vxdwraps.o vxd/fs.o vxd/fault.o vxd/entry.o vxd/console.o vxd/v86_api.o vxd/services.o
vxd-deps = $(vxd-objs:.o=.d)
vxd-nasm-flags = -Iddk -Ivxd -Iinc -felf32 -w-pp-macro-params-legacy
vxd-cc = i386-linux-musl-gcc
vxd-c-flags = $(CFLAGS) -ffreestanding -nostdlib -nostdinc -fno-exceptions -funsigned-char -DVXD $(wsl9x-include)

com-nasm-flags = -Iinc -fbin

.PHONY: all
all: hdd.img

.PHONY: clean
clean:
	rm -f wsl.com wsl9x.vxd vmlinux.elf $(vxd-objs) $(vxd-deps)

.PHONY: bochs
bochs: hdd.img
	bochs -qf bochs.bxrc -debugger -rc <(echo c)

.PHONY: gdb
gdb: hdd.img
gdb: export PATH := $(BOCHSGDB)/bin:$(PATH)
gdb:
	bochs -qf bochs-gdb.bxrc

.PHONY: compile_commands.json
compile_commands.json: clean
	bear -- make wsl9x.vxd

.PHONY: hdd.img
hdd.img: export MTOOLSRC=mtoolsrc
hdd.img: hdd.base.img wsl.com wsl9x.vxd vmlinux.elf config.sys system.ini
	cp hdd.base.img $@
	rm -f $@.lock
	mcopy -Do wsl9x.vxd c:/windows/system/WSL9X.VXD
	mcopy -Do config.sys c:/
	mcopy -Do wsl.com c:/windows/
	mcopy -Do system.ini c:/windows/
	mcopy -Do vmlinux.elf C:/VMLINUX.ELF

wsl9x.vxd: $(vxd-objs) vxd/link.def fixlink
	bin/wlink @vxd/link.def $(vxd-objs:%=file %)
	./fixlink -vxd32 $@

wsl.com: wsl/wsl.asm
	nasm $(com-nasm-flags) -o $@ $<

vxd/%.o: vxd/%.asm
	nasm $(vxd-nasm-flags) -MD $(@:.o=.d) -o $@ $<

vxd/%.o: vxd/%.c
	$(vxd-cc) $(vxd-c-flags) -MD -o $@ -c $<

fixlink: fixlink.c
	gcc -std=c89 -Wall -Dstricmp=strcasecmp -include strings.h -o $@ $<

vmlinux.elf: $(LINUX)/vmlinux
	strip -o $@ $<

.PHONY: build-linux
build-linux:
	make -C $(LINUX) win9x_defconfig ARCH=um SUBARCH=i386
	make -C $(LINUX) vmlinux ARCH=um SUBARCH=i386

.PHONY: build-linux
clean-linux:
	make -C $(LINUX) clean

-include $(vxd-deps)
