#include "loader.h"
#include "elf.h"
#include "fs.h"
#include "prelude.h"
#include "vxdwraps.h"

#include <wsl9x/mem.h>

static const u32 KERNEL_ADDR = 0xd0000000;
static const u32 KERNEL_SIZE = 0x68e000;
static const u32 PAGE_SIZE = 0x1000;
static const u32 BUFFER_SIZE = 0x8000;

typedef void(*kernel_entry_t)(void);

u32 kernel_base = 0;
kernel_entry_t kernel_entry = NULL;

bool wsl9x_reserve_addresses(void)
{
    // request a specific address and fail to load if we can't get it.
    // eventually we should do some relocation stuff
    u32 request = KERNEL_ADDR / PAGE_SIZE;

    kernel_base = VMM_PageReserve(request, KERNEL_SIZE / PAGE_SIZE, PR_FIXED | PR_4MEG);
    printf("VMM_PageReserve => %lx\n", kernel_base);

    if (kernel_base == (u32)-1) {
        printf("VMM_PageReserve failed\n");
        return false;
    }

    return true;
}

static bool allocate_memory(void)
{
    u32 free_pages = VMM_GetFreePageCount(0);
    printf("VMM_GetFreePageCount => %lx\n", free_pages);

    u32 res = VMM_PageCommit(kernel_base / PAGE_SIZE, KERNEL_SIZE / PAGE_SIZE, PD_FIXED, 0, PC_FIXED | PC_WRITEABLE);
    printf("VMM_PageCommit => %lx\n", res);
    if (!res) {
        printf("VMM_PageCommit failed\n");
        return false;
    }

    return true;
}

static void* temp_data_ptr = NULL;
static u16 temp_data_seg = 0;

static bool allocate_temp_data(void)
{
    u32 linear = VMM_Allocate_Temp_V86_Data_Area(BUFFER_SIZE, 0);

    if (linear > 0xfffff) {
        printf("VMM_Allocate_Temp_V86_Data_Area returned linear address > 0xfffff ?\n");
        return false;
    }

    printf("wsl9x: allocate_temp_data => %lx\n", linear);

    temp_data_ptr = (void*)linear;
    temp_data_seg = linear >> 4;

    return true;
}

static void free_temp_data(void)
{
    VMM_Free_Temp_V86_Data_Area();
    temp_data_ptr = 0;
    temp_data_seg = 0;
}

static bool read_struct(FileHandle kernel, void* out, size_t size)
{
    ssize_t rc = dos_read(kernel, temp_data_seg, size);
    if (rc < 0 || (size_t)rc != size) {
        printf("loader: read_struct err: %ld", rc);
        return false;
    }

    memcpy32(out, temp_data_ptr, size);
    return true;
}

static bool load_segment(FileHandle kernel, Elf32_Phdr* phdr)
{
    if (phdr->p_type != PT_LOAD) {
        return true;
    }

    printf("wsl9x: LOAD 0x%08lx: size %lu, file offset %lu, flags %lx\n",
        phdr->p_vaddr,
        phdr->p_filesz,
        phdr->p_offset,
        phdr->p_flags);

    if (phdr->p_filesz != phdr->p_memsz) {
        printf("  filesz != memsz\n");
        return false;
    }

    if (dos_seek(kernel, phdr->p_offset)) {
        printf("  seek error: offset=%lu\n", phdr->p_offset);
        return false;
    }

    u32 offset = 0;
    u8* load_ptr = (u8*)phdr->p_vaddr;

    while (offset < phdr->p_memsz) {
        u32 size = phdr->p_memsz - offset;
        if (size > BUFFER_SIZE) {
            size = BUFFER_SIZE;
        }

        ssize_t rc = dos_read(kernel, temp_data_seg, (u16)size);
        if (rc != (ssize_t)size) {
            printf("  read error: %ld\n", rc);
            return false;
        }

        printf(".");
        memcpy32(load_ptr, temp_data_ptr, size);
        load_ptr += size;
        offset += size;
    }

    return true;
}

static bool load_elf(FileHandle kernel)
{
    ssize_t rc;

    Elf32_Ehdr ehdr;
    if (!read_struct(kernel, &ehdr, sizeof(ehdr))) {
        return false;
    }

    if (ehdr.e_machine != EM_386) {
        printf("wsl9x loader: wrong machine: 0x%hx\n", ehdr.e_machine);
        return false;
    }

    if (ehdr.e_phentsize != sizeof(Elf32_Phdr)) {
        printf("wsl9x loader: wrong phentsize: 0x%x\n", ehdr.e_phentsize);
        return false;
    }

    kernel_entry = (kernel_entry_t)ehdr.e_entry;

    for (u32 i = 0; i < (u32)ehdr.e_phnum; i++) {
        u32 phoff = ehdr.e_phoff + i * sizeof(Elf32_Phdr);
        printf("phoff=%lu\n", phoff);

        if ((rc = dos_seek(kernel, phoff))) {
            printf("wsl9x loader: seek error: rc=%ld, phoff=%lu\n", rc, phoff);
            return false;
        }

        Elf32_Phdr phdr;
        if (!read_struct(kernel, &phdr, sizeof(phdr))) {
            printf("wsl9x loader: read phdr error\n");
        }

        if (!load_segment(kernel, &phdr)) {
            return false;
        }
    }

    return true;
}

bool wsl9x_load_kernel(void)
{
    if (!allocate_memory()) {
        return false;
    }

    const char* kernel_path = "\\VMLINUX.ELF";
    FileHandle vmlinux = dos_open(kernel_path);
    if (!vmlinux) {
        printf("wsl9x_load_kernel: open %s failed\n", kernel_path);
        return false;
    }

    printf("Opened vmlinux.elf: %lx\n", (u32)vmlinux);

    allocate_temp_data();
    bool ok = load_elf(vmlinux);
    free_temp_data();

    dos_close(vmlinux);

    if (!ok) {
        return false;
    }

    __asm__ volatile("xchgw %bx, %bx");
    kernel_entry();
    return true;
}

