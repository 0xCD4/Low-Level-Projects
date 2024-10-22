#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PHYSICAL_MEMORY_SIZE 1024
#define PHYSICAL_FRAMES 16
#define VIRTUAL_PAGES 64
#define FRAME_SIZE (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES)
#define PAGE_SIZE (PHYSICAL_MEMORY_SIZE / VIRTUAL_PAGES)
#define VIRTUAL_MEMORY_SIZE 256

typedef struct {
    int frame_number;
    bool is_valid;     // Track if page is valid
    unsigned int last_access;  // For LRU implementation
} PageTableEntry;

typedef struct {
    unsigned char *physical_memory;  // Dynamic allocation for physical memory
    PageTableEntry *page_table;     // Dynamic allocation for page table
    bool *free_frames;              // Dynamic allocation for frame tracking
    unsigned int access_count;      // Track memory accesses
} RamSimulator;

// Inline function for address translation
static inline unsigned int get_physical_address(int frame_number, unsigned int offset) {
    return frame_number * FRAME_SIZE + offset;
}

// Inline functions for page number and offset calculation
static inline unsigned int get_page_number(unsigned int virtual_address) {
    return virtual_address / PAGE_SIZE;
}

static inline unsigned int get_offset(unsigned int virtual_address) {
    return virtual_address % FRAME_SIZE;
}

static inline bool is_address_valid(unsigned int address) {
    return address < VIRTUAL_MEMORY_SIZE;
}

int find_free_frame(RamSimulator *ram) {
    for (int i = 0; i < PHYSICAL_FRAMES; i++) {
        if (!ram->free_frames[i]) {
            return i;
        }
    }
    
    // Implement LRU replacement if no free frames
    unsigned int oldest_access = -1;
    int victim_frame = 0;
    
    for (int i = 0; i < VIRTUAL_PAGES; i++) {
        if (ram->page_table[i].is_valid && ram->page_table[i].last_access < oldest_access) {
            oldest_access = ram->page_table[i].last_access;
            victim_frame = ram->page_table[i].frame_number;
            ram->page_table[i].is_valid = false;
        }
    }
    
    return victim_frame;
}

bool initialize_simulator(RamSimulator *ram) {
    // Allocate memory dynamically
    ram->physical_memory = (unsigned char *)calloc(PHYSICAL_MEMORY_SIZE, sizeof(unsigned char));
    ram->page_table = (PageTableEntry *)calloc(VIRTUAL_PAGES, sizeof(PageTableEntry));
    ram->free_frames = (bool *)calloc(PHYSICAL_FRAMES, sizeof(bool));
    
    if (!ram->physical_memory || !ram->page_table || !ram->free_frames) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }

    // Initialize page table entries
    for (int i = 0; i < VIRTUAL_PAGES; i++) {
        ram->page_table[i].frame_number = -1;
        ram->page_table[i].is_valid = false;
        ram->page_table[i].last_access = 0;
    }

    ram->access_count = 0;
    printf("RAM simulator initialized successfully\n");
    return true;
}

bool write_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char data) {
    if (!is_address_valid(virtual_address)) {
        fprintf(stderr, "Error: Virtual address %u is out of bounds\n", virtual_address);
        return false;
    }

    unsigned int page_number = get_page_number(virtual_address);
    unsigned int offset = get_offset(virtual_address);
    
    // Page fault handling
    if (!ram->page_table[page_number].is_valid) {
        int free_frame = find_free_frame(ram);
        if (free_frame == -1) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return false;
        }
        
        ram->page_table[page_number].frame_number = free_frame;
        ram->page_table[page_number].is_valid = true;
        ram->free_frames[free_frame] = true;
        printf("Page fault handled: Page %u mapped to frame %d\n", page_number, free_frame);
    }
    
    // Update access information
    ram->page_table[page_number].last_access = ++ram->access_count;
    
    // Write data to physical memory
    unsigned int physical_address = get_physical_address(
        ram->page_table[page_number].frame_number, 
        offset
    );
    
    ram->physical_memory[physical_address] = data;
    return true;
}

bool read_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char *data) {
    if (!is_address_valid(virtual_address)) {
        fprintf(stderr, "Error: Virtual address %u is out of bounds\n", virtual_address);
        return false;
    }

    unsigned int page_number = get_page_number(virtual_address);
    unsigned int offset = get_offset(virtual_address);
    
    if (!ram->page_table[page_number].is_valid) {
        fprintf(stderr, "Error: Page fault - page %u not in memory\n", page_number);
        return false;
    }
    
    // Update access information
    ram->page_table[page_number].last_access = ++ram->access_count;
    
    // Read data from physical memory
    unsigned int physical_address = get_physical_address(
        ram->page_table[page_number].frame_number, 
        offset
    );
    
    *data = ram->physical_memory[physical_address];
    return true;
}

void cleanup_simulator(RamSimulator *ram) {
    free(ram->physical_memory);
    free(ram->page_table);
    free(ram->free_frames);
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
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                printf("Enter virtual address (0-255): ");
                if (scanf("%u", &address) != 1 || !is_address_valid(address)) {
                    printf("Invalid address. Please enter a value between 0 and 255\n");
                    while (getchar() != '\n');
                    break;
                }
                printf("Enter value to write (0-255): ");
                if (scanf("%hhu", &data) != 1) {
                    printf("Invalid value. Please enter a value between 0 and 255\n");
                    while (getchar() != '\n');
                    break;
                }
                if (write_memory(&ram, address, data)) {
                    printf("Data %u written to virtual address %u\n", data, address);
                }
                break;

            case 2:
                printf("Enter virtual address (0-255): ");
                if (scanf("%u", &address) != 1 || !is_address_valid(address)) {
                    printf("Invalid address. Please enter a value between 0 and 255\n");
                    while (getchar() != '\n');
                    break;
                }
                if (read_memory(&ram, address, &data)) {
                    printf("Data at virtual address %u is %u\n", address, data);
                }
                break;

            case 3:
                printf("Cleaning up and exiting...\n");
                cleanup_simulator(&ram);
                return EXIT_SUCCESS;

            default:
                printf("Invalid choice. Please select a valid operation\n");
                break;
        }
    }
}