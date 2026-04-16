ifndef LINUX
LINUX = $(error must define LINUX env var)
endif

CFLAGS = -std=c11 -Wall -Wextra -Wstrict-prototypes -funsigned-char

wsl9x-include = \
	-I $(LINUX)/include \
	-I $(LINUX)/arch/um/os-Win95/include

vxd-objs = vxd/wsl9x.o vxd/loader.o vxd/vxdwraps.o vxd/fs.o vxd/fault.o
vxd-deps = $(vxd-objs:.o=.d)
vxd-nasm-flags = -Iddk -Ivxd -felf32 -w-pp-macro-params-legacy
vxd-cc = i386-linux-musl-gcc
vxd-c-flags = $(CFLAGS) -ffreestanding -nostdlib -nostdinc -DVXD $(wsl9x-include) -fno-exceptions

.PHONY: all
all: hdd.img

.PHONY: clean
clean:
	rm -f wsl9x.vxd vmlinux.elf $(vxd-objs) $(vxd-deps)

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
hdd.img: hdd.base.img wsl9x.vxd vmlinux.elf config.sys
hdd.img: export MTOOLSRC=mtoolsrc
hdd.img:
	cp hdd.base.img $@
	rm -f $@.lock
	mcopy -Do wsl9x.vxd c:/windows/system/WSL9X.VXD
	mcopy -Do config.sys c:/
	mcopy -Do system.ini c:/windows/
	mcopy -Do vmlinux.elf C:/VMLINUX.ELF

wsl9x.vxd: $(vxd-objs) vxd/link.def fixlink/fixlink
	wlink @vxd/link.def $(vxd-objs:%=file %)
	fixlink/fixlink -vxd32 $@

vxd/%.o: vxd/%.asm
	nasm $(vxd-nasm-flags) -MD $(@:.o=.d) -o $@ $<

vxd/%.o: vxd/%.c
	$(vxd-cc) $(vxd-c-flags) -MD -o $@ -c $<

fixlink/fixlink: fixlink/fixlink.c
	gcc -std=c89 -Wall -Dstricmp=strcasecmp -include strings.h -o $@ $<

vmlinux.elf: vmlinux
	strip -o $@ $<

-include $(vxd-deps)
