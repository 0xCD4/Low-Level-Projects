#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#define PHYSICAL_MEMORY_SIZE 1024
#define PHYSICAL_FRAMES 16
#define VIRTUAL_PAGES 64
#define ROWHAMMER_THRESHOLD 1000
#define PAGE_SIZE (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES)

typedef struct {
    int frame_number;
    bool valid;
    uint8_t padding[3];
} PageTableEntry;

typedef struct {
    uint8_t PHYSICAL_MEMORY[PHYSICAL_MEMORY_SIZE];
    PageTableEntry page_table[VIRTUAL_PAGES];
    uint16_t free_frames_bitmap;
    uint32_t access_count[PHYSICAL_FRAMES];
    time_t last_access_time[PHYSICAL_FRAMES];
    uint64_t performance_stats[4]; // hits, misses, reads, writes
} RamSimulator;

static inline bool is_frame_free(uint16_t bitmap, int frame) {
    return !(bitmap & (1 << frame));
}

static inline void mark_frame_used(uint16_t* bitmap, int frame) {
    *bitmap |= (1 << frame);
}

static inline void mark_frame_free(uint16_t* bitmap, int frame) {
    *bitmap &= ~(1 << frame);
}

bool initialize_simulator(RamSimulator* ram) {
    if (!ram) return false;
    
    memset(ram->PHYSICAL_MEMORY, 0, PHYSICAL_MEMORY_SIZE);
    for (int i = 0; i < VIRTUAL_PAGES; i++) {
        ram->page_table[i].frame_number = -1;
        ram->page_table[i].valid = false;
    }
    
    ram->free_frames_bitmap = 0;
    memset(ram->access_count, 0, sizeof(uint32_t) * PHYSICAL_FRAMES);
    memset(ram->last_access_time, 0, sizeof(time_t) * PHYSICAL_FRAMES);
    memset(ram->performance_stats, 0, sizeof(uint64_t) * 4);
    
    return true;
}

int find_free_frame(RamSimulator* ram) {
    if (!ram) return -1;
    
    if (ram->free_frames_bitmap == 0xFFFF) return -1;
    
    for (int i = 0; i < PHYSICAL_FRAMES; i++) {
        if (is_frame_free(ram->free_frames_bitmap, i)) {
            mark_frame_used(&ram->free_frames_bitmap, i);
            return i;
        }
    }
    return -1;
}

void check_rowhammer(RamSimulator* ram, int frame_number) {
    time_t current_time = time(NULL);
    if (ram->access_count[frame_number] > ROWHAMMER_THRESHOLD &&
        difftime(current_time, ram->last_access_time[frame_number]) < 1.0) {
        printf("WARNING: Potential Rowhammer attack detected on frame %d!\n", frame_number);
        printf("Access count: %d\n", ram->access_count[frame_number]);
    }
    ram->last_access_time[frame_number] = current_time;
}

bool write_memory(RamSimulator* ram, unsigned int virtual_address, unsigned char data) {
    if (!ram || virtual_address >= PHYSICAL_MEMORY_SIZE) return false;
    
    unsigned int page_number = virtual_address / PAGE_SIZE;
    unsigned int offset = virtual_address % PAGE_SIZE;
    
    ram->performance_stats[3]++; // Increment writes
    
    if (!ram->page_table[page_number].valid) {
        int free_frame = find_free_frame(ram);
        if (free_frame == -1) {
            printf("Error: No free frames available\n");
            ram->performance_stats[1]++; // Increment misses
            return false;
        }
        ram->page_table[page_number].frame_number = free_frame;
        ram->page_table[page_number].valid = true;
    }
    
    int frame_number = ram->page_table[page_number].frame_number;
    ram->PHYSICAL_MEMORY[frame_number * PAGE_SIZE + offset] = data;
    ram->access_count[frame_number]++;
    check_rowhammer(ram, frame_number);
    ram->performance_stats[0]++; // Increment hits
    
    return true;
}

bool read_memory(RamSimulator* ram, unsigned int virtual_address, unsigned char* data) {
    if (!ram || !data || virtual_address >= PHYSICAL_MEMORY_SIZE) return false;
    
    unsigned int page_number = virtual_address / PAGE_SIZE;
    unsigned int offset = virtual_address % PAGE_SIZE;
    
    ram->performance_stats[2]++; // Increment reads
    
    if (!ram->page_table[page_number].valid) {
        printf("Error: Page fault - address not mapped\n");
        ram->performance_stats[1]++; // Increment misses
        return false;
    }
    
    int frame_number = ram->page_table[page_number].frame_number;
    *data = ram->PHYSICAL_MEMORY[frame_number * PAGE_SIZE + offset];
    ram->access_count[frame_number]++;
    check_rowhammer(ram, frame_number);
    ram->performance_stats[0]++; // Increment hits
    
    return true;
}

void print_stats(RamSimulator* ram) {
    if (!ram) return;
    printf("\nPerformance Statistics:\n");
    printf("Cache Hits: %lu\n", ram->performance_stats[0]);
    printf("Cache Misses: %lu\n", ram->performance_stats[1]);
    printf("Total Reads: %lu\n", ram->performance_stats[2]);
    printf("Total Writes: %lu\n", ram->performance_stats[3]);
    float hit_rate = 0;
    if (ram->performance_stats[0] + ram->performance_stats[1] > 0) {
        hit_rate = (float)ram->performance_stats[0] / 
                   (ram->performance_stats[0] + ram->performance_stats[1]) * 100;
    }
    printf("Hit Rate: %.2f%%\n", hit_rate);
}

int main(void) {
    RamSimulator ram;
    
    if (!initialize_simulator(&ram)) {
        fprintf(stderr, "Failed to initialize RAM simulator\n");
        return EXIT_FAILURE;
    }
    
    int choice;
    unsigned int address;
    unsigned char data;
    char input_buffer[32];
    
    while (1) {
        printf("\nRAM Simulator Menu:\n");
        printf("1. Write to RAM\n");
        printf("2. Read from RAM\n");
        printf("3. Show Statistics\n");
        printf("4. Exit\n");
        printf("Enter choice (1-4): ");
        
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf("Error reading input\n");
            continue;
        }
        
        if (sscanf(input_buffer, "%d", &choice) != 1) {
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
                while (getchar() != '\n');
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
                while (getchar() != '\n');
                break;
                
            case 3:
                print_stats(&ram);
                break;
                
            case 4:
                printf("Exiting...\n");
                return EXIT_SUCCESS;
                
            default:
                printf("Invalid choice. Please enter 1-4.\n");
                break;
        }
    }
    
    return EXIT_SUCCESS;
}