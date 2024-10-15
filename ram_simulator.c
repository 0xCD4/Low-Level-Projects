#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define RAM_SIZE (1024 * 1024) // 1MB
#define ROWHAMMER_THRESHOLD 10000

typedef enum{

    READ_ONLY,
    READ_WRITE,
} MemoryProtection;

typedef struct {
    int start_address; 
    int end_address; 
    MemoryProtection protection; 
} MemoryRegion;

/**
 * @file ram_simulator.c
 * @brief This file contains the implementation of a RAM simulator.
 *
 * The RAM simulator is designed to emulate the behavior of memory regions.
 * It includes the definition and initialization of memory regions.
 */

/**
 * @brief Array of memory regions.
 *
 * This array holds the different memory regions that are part of the RAM simulator.
 * Each element in the array represents a distinct memory region with its own properties.
 */
MemoryRegion regions[] = {
    {0x00000000, 0x0000FFFF, READ_ONLY},  // 0 - 65535  
    {0x00010000, 0x000FFFFF, READ_WRITE}  // 65536 - 1048575
};

typedef struct {
    uint32_t address;
    int value;
} MemoryAccess;

uint8_t ram[RAM_SIZE] = {0};
int access_count[RAM_SIZE] = {0};

int check_protection(uint32_t address, MemoryProtection protection){
    for(int i = 0; i < sizeof(regions) / sizeof(MemoryRegion); i++){
        if(address >= regions[i].start_address && address <= regions[i].end_address){
            return regions[i].protection == protection;
        }
    }
    return 0;
}



void check_rowhammer(uint32_t address) {
    access_count[address]++;
    if (access_count[address] > ROWHAMMER_THRESHOLD) {
        printf("Warning: Potential Rowhammer attack detected at address 0x%08X\n", address);
    }
}


void write_ram(uint32_t address, int value){
    if (address >= RAM_SIZE) {
        printf("Error: Address out of range\n");
        return;
    }
    if(check_protection(address, READ_WRITE)){
        ram[address] = value;
        printf("Data written to memory cell %d\n", address);
    }else{
        printf("Error: Write to read-only memory\n");
        exit(EXIT_FAILURE);
    }
}

int read_ram(uint32_t address){

    if(address >= RAM_SIZE){
        printf("Error: Address out of range\n");
        return -1;
    }
        for(int i = 0; i < sizeof(regions) / sizeof(MemoryRegion); i++){
            if(address >= regions[i].start_address && address <= regions[i].end_address){
                return ram[address];
            }
        }
        return -1;
    }


int main(){

    uint32_t address;
    int value;

    printf("Enter memory address to write to (in hex, e.g., 0x00010000): ");
    scanf("%x", &address);
    printf("Enter value to write: ");
    scanf("%d", &value);

    write_ram(address, value);

    int data = read_ram(0x00010000); // read from memory cell 65536
    printf("Data at address 0x00010000: %d\n", data);
    //for(int i = 0; i < sizeof(regions) / sizeof(MemoryRegion); i++){
      //  printf("Region %d: Start Address: 0x%08X, End Address: 0x%08X, Protection: %s\n", i, regions[i].start_address, regions[i].end_address, regions[i].protection == READ_ONLY ? "Read-Only" : "Read-Write");
    //}

}