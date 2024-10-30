#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define PHYSICAL_MEMORY_SIZE 1024
#define PHYSICAL_FRAMES 16
#define VIRTUAL_PAGES 64
#define ROWHAMMER_THRESHOLD 1000 // Example threshold for Rowhammer detection

typedef struct {
    int frame_number;
} PageTableEntry;

typedef struct {
    unsigned char PHYSICAL_MEMORY[PHYSICAL_MEMORY_SIZE]; // physical memory
    PageTableEntry page_table[VIRTUAL_PAGES]; // page table
    int free_frames[PHYSICAL_FRAMES]; // free frames in physical memory
    int access_count[PHYSICAL_FRAMES]; // access count for each frame
    time_t last_access_time[PHYSICAL_FRAMES]; // last access time for each frame
} RamSimulator;

bool initialize_simulator(RamSimulator *ram) {
    for (int i = 0; i < PHYSICAL_MEMORY_SIZE; i++) {
        ram->PHYSICAL_MEMORY[i] = 0; // initialize the physical memory to 0
    }
    printf("RAM simulator initialized\n");

    for (int i = 0; i < VIRTUAL_PAGES; i++) {
        ram->page_table[i].frame_number = -1; // page is not mapped
    }

    for (int i = 0; i < PHYSICAL_FRAMES; i++) {
        ram->free_frames[i] = 0; // all frames are initially free
        ram->access_count[i] = 0; // initialize access count
        ram->last_access_time[i] = 0; // initialize last access time
    }

    return true;
}

int find_free_frame(RamSimulator *ram) {
    for (int i = 0; i < PHYSICAL_FRAMES; i++) {
        if (ram->free_frames[i] == 0) {
            return i;
        }
    }
    return -1; // no free frame available
}

void check_rowhammer(RamSimulator *ram, int frame_number) {
    time_t current_time = time(NULL);
    if (ram->access_count[frame_number] > ROWHAMMER_THRESHOLD &&
        difftime(current_time, ram->last_access_time[frame_number]) < 1) {
        printf("Warning: Potential Rowhammer attack detected on frame %d\n", frame_number);
    }
    ram->last_access_time[frame_number] = current_time;
}

bool write_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char data) {
    unsigned int page_number = virtual_address / (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES);
    unsigned int offset = virtual_address % (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES);

    if (page_number >= VIRTUAL_PAGES) {
        printf("Error: Page number %u is out of bounds\n", page_number);
        return false;
    }

    if (ram->page_table[page_number].frame_number == -1) {
        int free_frame = find_free_frame(ram);
        if (free_frame == -1) {
            printf("Error: No free frame available\n");
            return false;
        }
        ram->page_table[page_number].frame_number = free_frame;
        ram->free_frames[free_frame] = 1; // mark this frame as used
    }

    int frame_number = ram->page_table[page_number].frame_number;
    ram->PHYSICAL_MEMORY[frame_number * (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES) + offset] = data;
    ram->access_count[frame_number]++;
    check_rowhammer(ram, frame_number);

    return true;
}

bool read_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char *data) {
    unsigned int page_number = virtual_address / (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES);
    unsigned int offset = virtual_address % (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES);

    if (page_number >= VIRTUAL_PAGES) {
        printf("Error: Page number %u is out of bounds\n", page_number);
        return false;
    }

    if (ram->page_table[page_number].frame_number == -1) {
        printf("Error: Page number %u is not mapped to any frame\n", page_number);
        return false;
    }

    int frame_number = ram->page_table[page_number].frame_number;
    *data = ram->PHYSICAL_MEMORY[frame_number * (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES) + offset];
    ram->access_count[frame_number]++;
    check_rowhammer(ram, frame_number);

    return true;
}

int main() {
    RamSimulator ram;

    if (!initialize_simulator(&ram)) {
        fprintf(stderr, "RAM simulator initialization failed\n");
        return EXIT_FAILURE;
    }

    int choice;
    unsigned int address;
    unsigned char data;

    while (1) {
        printf("\nSelect an option:\n");
        printf("1. Write to RAM\n");
        printf("2. Read from RAM\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid choice\n");
            break;
        }
        switch (choice) {
            case 1:
                printf("Enter virtual address (0-255): ");
                if (scanf("%u", &address) != 1 || address >= PHYSICAL_MEMORY_SIZE) {
                    printf("Invalid address. Please enter a value between 0 and 255\n");
                    break;
                }
                printf("Enter value to write (0-255): ");
                if (scanf("%hhu", &data) != 1) {
                    printf("Invalid data. Please enter a value between 0 and 255\n");
                    break;
                }
                if (write_memory(&ram, address, data)) {
                    printf("Successfully written %u to address %u\n", data, address);
                }
                break;
            case 2:
                printf("Enter virtual address (0-255): ");
                if (scanf("%u", &address) != 1 || address >= PHYSICAL_MEMORY_SIZE) {
                    printf("Invalid address. Please enter a value between 0 and 255\n");
                    break;
                }
                if (read_memory(&ram, address, &data)) {
                    printf("Data at address %u is %u\n", address, data);
                }
                break;
            case 3:
                printf("Exiting...\n");
                return EXIT_SUCCESS;
            default:
                printf("Invalid choice. Please select a valid operation\n");
                break;
        }
    }

    return EXIT_SUCCESS;
}