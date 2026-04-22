# WSL9x

Windows 9x Subsystem for Linux.

WSL9x runs a modern Linux kernel (6.19 at time of writing) cooperatively inside the Windows 9x kernel, enabling users to take advantage of the full suite of capabilities of both operating systems at the same time, including paging, memory protection, and pre-emptive scheduling. Run all your favourite applications side by side - no rebooting required!

Proudly written without AI.

![](screenshot.png)

## Building and running

* You will need a cross toolchain targeting `i386-linux-musl` on PATH. Use [musl-cross-make](https://github.com/richfelker/musl-cross-make) to build one

* You will need the Open Watcom v2 toolchain for building the Windows components

* Build a patched Linux kernel from my [`win9x-um-6.19` branch](https://github.com/haileys/linux/tree/win9x-um-6.19)

   ```sh-session
   $ make defconfig ARCH=um SUBARCH=i386 KBUILD_DEFCONFIG=win9x
   $ make vmlinux ARCH=um SUBARCH=i386 -j $(nproc)
   ```

* Set env vars appropriately for the WSL9x build, in particular you will need to point `WATCOM` and `LINUX` at the right places. See `.envrc.example` for an example

* You will need a hard drive image `hdd.base.img` with Windows 9x pre-installed

* Run `make` - this will produce a new `hdd.img` with WSL9x ready to go.

* Run `wsl` at the MS-DOS prompt to open a pty. If you'd like to use ANSI colours, make sure you have an appropriate driver loaded before running `wsl`. [`nnansi.com`](https://www.kegel.com/nansi/) is a good option.

## License

GPL-3
