#include <stdio.h>

#define RAM_SIZE 1024

int ram[RAM_SIZE];

void write_ram(int address, int data) {
    if (address >= 0 && address < RAM_SIZE) {
        ram[address] = data;
        printf("Data %d written to address %d\n", data, address);
    } else {
        printf("Error: Address %d is out of the bounds\n", address);
    }
}