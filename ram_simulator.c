#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define PHYSICAL_MEMORY_SIZE 1024
#define PHYSICAL_FRAMES 16
#define VIRTUAL_PAGES 64
#define ROWHAMMER_THRESHOLD 1000
#define PAGE_SIZE (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES)

typedef struct {
    int frame_number;
    bool valid;
} PageTableEntry;

typedef struct {
    unsigned char PHYSICAL_MEMORY[PHYSICAL_MEMORY_SIZE];
    PageTableEntry page_table[VIRTUAL_PAGES];
    int free_frames[PHYSICAL_FRAMES];
    int access_count[PHYSICAL_FRAMES];
    time_t last_access_time[PHYSICAL_FRAMES];
} RamSimulator;

bool initialize_simulator(RamSimulator *ram) {
    memset(ram->PHYSICAL_MEMORY, 0, PHYSICAL_MEMORY_SIZE);
    
    for (int i = 0; i < VIRTUAL_PAGES; i++) {
        ram->page_table[i].frame_number = -1;
        ram->page_table[i].valid = false;
    }
    
    for (int i = 0; i < PHYSICAL_FRAMES; i++) {
        ram->free_frames[i] = 0;
        ram->access_count[i] = 0;
        ram->last_access_time[i] = 0;
    }
    
    printf("RAM simulator initialized\n");
    return true;
}

int find_free_frame(RamSimulator *ram) {
    for (int i = 0; i < PHYSICAL_FRAMES; i++) {
        if (ram->free_frames[i] == 0) return i;
    }
    return -1;
}

void check_rowhammer(RamSimulator *ram, int frame_number) {
    time_t current_time = time(NULL);
    if (ram->access_count[frame_number] > ROWHAMMER_THRESHOLD &&
        difftime(current_time, ram->last_access_time[frame_number]) < 1) {
        printf("WARNING: Potential Rowhammer attack detected on frame %d!\n", frame_number);
        printf("Access count: %d\n", ram->access_count[frame_number]);
    }
    ram->last_access_time[frame_number] = current_time;
}

bool write_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char data) {
    unsigned int page_number = virtual_address / PAGE_SIZE;
    unsigned int offset = virtual_address % PAGE_SIZE;
    
    if (page_number >= VIRTUAL_PAGES) {
        printf("Error: Page number %u is out of bounds\n", page_number);
        return false;
    }
    
    if (!ram->page_table[page_number].valid) {
        int free_frame = find_free_frame(ram);
        if (free_frame == -1) {
            printf("Error: No free frames available\n");
            return false;
        }
        ram->page_table[page_number].frame_number = free_frame;
        ram->page_table[page_number].valid = true;
        ram->free_frames[free_frame] = 1;
    }
    
    int frame_number = ram->page_table[page_number].frame_number;
    ram->PHYSICAL_MEMORY[frame_number * PAGE_SIZE + offset] = data;
    ram->access_count[frame_number]++;
    check_rowhammer(ram, frame_number);
    
    return true;
}

bool read_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char *data) {
    unsigned int page_number = virtual_address / PAGE_SIZE;
    unsigned int offset = virtual_address % PAGE_SIZE;
    
    if (page_number >= VIRTUAL_PAGES) {
        printf("Error: Page number %u is out of bounds\n", page_number);
        return false;
    }
    
    if (!ram->page_table[page_number].valid) {
        printf("Error: Page fault - address not mapped\n");
        return false;
    }
    
    int frame_number = ram->page_table[page_number].frame_number;
    *data = ram->PHYSICAL_MEMORY[frame_number * PAGE_SIZE + offset];
    ram->access_count[frame_number]++;
    check_rowhammer(ram, frame_number);
    
    return true;
}

void show_memory_stats(RamSimulator *ram) {
    printf("\nMemory Statistics:\n");
    printf("Used frames: ");
    int used = 0;
    for (int i = 0; i < PHYSICAL_FRAMES; i++) {
        if (ram->free_frames[i]) {
            printf("%d ", i);
            used++;
        }
    }
    printf("\nTotal frames used: %d/%d\n", used, PHYSICAL_FRAMES);
}

int main() {
    RamSimulator ram;
    
    if (!initialize_simulator(&ram)) {
        fprintf(stderr, "Failed to initialize RAM simulator\n");
        return EXIT_FAILURE;}
    
    int choice;
    unsigned int address;
    unsigned char data;
    
    while (1) {
        printf("\nRAM Simulator Menu:\n");
        printf("1. Write to RAM\n");
        printf("2. Read from RAM\n");
        printf("3. Show Memory Stats\n");
        printf("4. Exit\n");
        printf("Enter choice (1-4): ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        
        switch (choice) {
            case 1:
                printf("Enter virtual address (0-%d): ", PHYSICAL_MEMORY_SIZE - 1);
                if (scanf("%u", &address) != 1 || address >= PHYSICAL_MEMORY_SIZE) {
                    printf("Invalid address\n");
                    while (getchar() != '\n');
                    break;
                }
                
                printf("Enter value (0-255): ");
                if (scanf("%hhu", &data) != 1) {
                    printf("Invalid data\n");
                    while (getchar() != '\n');
                    break;
                }
                
                if (write_memory(&ram, address, data)) {
                    printf("Successfully wrote %u to address %u\n", data, address);
                }
                break;
                
            case 2:
                printf("Enter virtual address (0-%d): ", PHYSICAL_MEMORY_SIZE - 1);
                if (scanf("%u", &address) != 1 || address >= PHYSICAL_MEMORY_SIZE) {
                    printf("Invalid address\n");
                    while (getchar() != '\n');
                    break;
                }
                
                if (read_memory(&ram, address, &data)) {
                    printf("Value at address %u: %u\n", address, data);
                }
                break;
                
            case 3:
                show_memory_stats(&ram);
                break;
                
            case 4:
                printf("Exiting RAM simulator...\n");
                return EXIT_SUCCESS;
                
            default:
                printf("Invalid choice. Please select 1-4.\n");
                break;
        }
    }
    
    return EXIT_SUCCESS;
}