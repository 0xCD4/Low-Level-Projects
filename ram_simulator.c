// RAM simulator in C
// virtual memory: 0-255    

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define PHYSICAL_MEMORY_SIZE 1024
#define PHYSICAL_FRAMES 16
#define VIRTUAL_PAGES 64


typedef struct{
    int frame_number;
}PageTableEntry;

typedef struct{

unsigned char PHYSICAL_MEMORY[PHYSICAL_MEMORY_SIZE]; // physical memory
PageTableEntry page_table[VIRTUAL_PAGES]; // page table
int free_frames[PHYSICAL_FRAMES]; // free frames in physical memory

}RamSimulator;


// we need to create a simulator that will read and write to the RAM

bool initialize_simulator(RamSimulator *ram){
    for(int i = 0; i < PHYSICAL_MEMORY_SIZE; i++){
        ram->PHYSICAL_MEMORY[i] = 0; // we will initialize the physical memory to 0
        int physical_address = ram->page_table[page_number].frame_number * (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES) + offset;
        ram->PHYSICAL_MEMORY[physical_address] = data;
        return true;
    }
    printf("RAM simulator initialized\n");
  

    // we need to create a page table  
    for(int i = 0;i<VIRTUAL_PAGES;i++){
        ram->page_table[i].frame_number = -1; // this indicates that the page is not mapped -1 
    }

    // we need to create a free frames array
    for(int i = 0;i<PHYSICAL_FRAMES;i++){
        ram->free_frames[i] = 0; // 0 indicates that the frame is free
    }

    return true;
}
bool write_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char data) {


bool write_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char data){
  
    unsigned int page_number = virtual_address / (PHYSICAL_MEMORY_SIZE / VIRTUAL_PAGES);
    unsigned int offset = virtual_address % (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES);

    if(page_number >= VIRTUAL_PAGES){
        printf("Error: Page number %u is out of bounds\n", page_number);
        return false;
    }

    // we should find free frame in physical memory
    if(ram->page_table[page_number].frame_number == -1){
        int free_frame = find_free_frame(ram);
        if(free_frame == -1){
            printf("Error: No free frame available\n");
            return false;
        }
        ram->page_table[page_number].frame_number = free_frame;
        ram->free_frames[free_frame] = 1; // we will mark this as used
        printf("Page number %u mapped to frame number %d\n", page_number, free_frame);
    }

}
bool read_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char *data) {
// read from memory 

bool read_memory(RamSimulator *ram, unsigned int virtual_address, unsigned char data) {
    unsigned int page_number = virtual_address / (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES);
    unsigned int offset = virtual_address % (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES);

    printf("Virtual Address: %u\n", virtual_address);
    printf("Page Number: %u\n", page_number);
    printf("Offset: %u\n", offset);

    if (page_number >= VIRTUAL_PAGES) {
        printf("Error: Page number %u is out of bounds\n", page_number);
        return false;
    }

    if (ram->page_table[page_number].frame_number == -1) {
        printf("Error: Page number %u is not mapped to any frame\n", page_number);
        return false;
    }

    int physical_address = ram->page_table[page_number].frame_number * (PHYSICAL_MEMORY_SIZE / PHYSICAL_FRAMES) + offset;
    printf("Physical Address: %d\n", physical_address);

    if (physical_address >= PHYSICAL_MEMORY_SIZE) {
        printf("Error: Physical address %d is out of bounds\n", physical_address);
        return false;
    }


}




int main(){

    // RAM simulator 
    RamSimulator ram;

    if(!initialize_simulator(&ram)){
        fprintf(stderr, "RAM simulator initialization failed\n");
        return EXIT_FAILURE;
    }

    int choice;
    unsigned int address;
    unsigned char data;

    while(1){
        printf("\nSelect an option:\n");
        printf("1. Write to RAM\n");
        printf("2. Read from RAM\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        
        if(scanf("%d", &choice) != 1){
            printf("Invalid input. Please enter a number.\n");
            while(getchar() != '\n'); // clear the input buffer
            continue;
        }

        switch(choice){
            case 1:
                printf("Enter virtual address (0-255): ");
                if(scanf("%u", &address) != 1 || address >= PHYSICAL_MEMORY_SIZE){
                    printf("Invalid address. Please enter a value between 0 and 255\n");
                    break;
                }
                printf("Enter value to write (0-255): ");
                if(scanf("%hhu", &data) != 1){
                    printf("Invalid value. Please enter a value between 0 and 255\n");
                    break;
                }
                if(write_memory(&ram, address, data)){
                    printf("Successfully written %u to address %u\n", data, address);
                }
                break;
            case 2:
                printf("Enter virtual address (0-255): ");
                if(scanf("%u", &address) != 1 || address >= 256){
                    printf("Invalid address. Please enter a value between 0 and 255\n");
                    break;
                }
                if(read_memory(&ram, address, &data)){
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