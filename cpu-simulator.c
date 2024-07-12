#include <stdio.h>
#include <stdlib.h>

#define MEMORY_SIZE 256
#define NUM_REGISTERS 4

typedef enum {
    ADD,
    SUB,
    LOAD,
    STORE,
    JUMP,
    JZ,
    JNZ,
    HALT
} Instruction;

typedef struct {
    int registers[NUM_REGISTERS];
    int pc; // Program Counter
    int memory[MEMORY_SIZE];
} CPU;

void execute_instruction(CPU *cpu, Instruction instr, int reg1, int reg2, int address) {
    printf("PC: %d, Executing: ", cpu->pc);
    switch (instr) {
        case ADD:
            printf("ADD R%d, R%d\n", reg1, reg2);
            cpu->registers[reg1] += cpu->registers[reg2];
            break;
        case SUB:
            printf("SUB R%d, R%d\n", reg1, reg2);
            cpu->registers[reg1] -= cpu->registers[reg2];
            break;
        case LOAD:
            printf("LOAD R%d, %d\n", reg1, address);
            cpu->registers[reg1] = cpu->memory[address];
            break;
        case STORE:
            printf("STORE R%d, %d\n", reg1, address);
            cpu->memory[address] = cpu->registers[reg1];
            break;
        case JUMP:
            printf("JUMP %d\n", address);
            cpu->pc = address - 4; // Adjust for automatic pc increment
            break;
        case JZ:
            printf("JZ R%d, %d\n", reg1, address);
            if (cpu->registers[reg1] == 0) {
                cpu->pc = address - 4; // Adjust for automatic pc increment
            }
            break;
        case JNZ:
            printf("JNZ R%d, %d\n", reg1, address);
            if (cpu->registers[reg1] != 0) {
                cpu->pc = address - 4; // Adjust for automatic pc increment
            }
            break;
        case HALT:
            printf("HALT\n");
            printf("Final State: R0=%d, R1=%d, R2=%d, R3=%d, Memory[0x20]=%d\n",
                   cpu->registers[0], cpu->registers[1], cpu->registers[2], cpu->registers[3], cpu->memory[0x20]);
            exit(0);
            break;
        default:
            printf("Unknown instruction.\n");
            exit(1);
    }
    cpu->pc += 4; // Move to the next instruction
}

void run_cpu(CPU *cpu, Instruction *program, int program_size) {
    while (cpu->pc < program_size) {
        Instruction instr = program[cpu->pc];
        int reg1 = program[cpu->pc + 1];
        int reg2 = program[cpu->pc + 2];
        int address = program[cpu->pc + 3];
        execute_instruction(cpu, instr, reg1, reg2, address);
        cpu->pc += 4; // Each instruction is 4 integers: instr, reg1, reg2, address
    }
}

int main() {
    CPU cpu = { .registers = {0}, .pc = 0, .memory = {0} };

    // LOAD R0, 0x10; ADD R0, R1; JZ R0, 0x18; SUB R0, R1; STORE R0, 0x20; JUMP 0x0C; HALT
    Instruction program[] = {
        LOAD, 0, 0, 0x10, // Load memory[0x10] into R0
        ADD, 0, 1, 0,    // Add R1 to R0
        JZ, 0, 0, 0x18,  // If R0 is zero, jump to instruction at address 0x18
        SUB, 0, 1, 0,    // Subtract R1 from R0
        STORE, 0, 0, 0x20, // Store R0 into memory[0x20]
        JUMP, 0, 0, 0x0C, // Jump to instruction at address 0x0C
        HALT, 0, 0, 0    // Halt execution
    };
    int program_size = sizeof(program) / sizeof(program[0]);

    // Initialize memory and registers
    cpu.memory[0x10] = 5;
    cpu.registers[1] = 1; // Set R1 to 1

    run_cpu(&cpu, program, program_size);

    printf("Memory[0x20]: %d\n", cpu.memory[0x20]);

    return 0;
}
