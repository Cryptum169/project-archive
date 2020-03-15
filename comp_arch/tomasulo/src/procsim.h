#ifndef PROCSIM_HPP
#define PROCSIM_HPP

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

// Default structure
#define DEFAULT_K0 3
#define DEFAULT_K1 1
#define DEFAULT_K2 2
#define DEFAULT_R 1
#define DEFAULT_F 4
#define DEFAULT_G 8
#define DEFAULT_C 10

// List of opcodes
typedef enum {
    OP_NOP = 1,
    OP_ADD = 2,
    OP_MUL = 3,
    OP_LOAD = 4,
    OP_STORE = 5,
    OP_BR = 6
} opcode_t;

// Configuration struct
typedef struct configuration {
    uint64_t f;     // Dispatch rate
    uint64_t k0;    // Number of k0 units
    uint64_t k1;    // Number of k1 units
    uint64_t k2;    // Number of k2 units
    uint64_t r;     // Number of reservation stations per FU type
    uint64_t g;     // log2(Number of entries in BTB)
    uint64_t c;     // log2(Size of data cache in bytes)
} proc_conf_t;

typedef struct instruction {
    uint64_t inst_addr;     // instruction address
    int32_t opcode;         // instruction opcode
    int32_t src_reg[2];     // Source register
    int32_t dest_reg;       // Destination register

    uint64_t ld_st_addr;    // LOAD / STORE address
    uint64_t br_target;     // Branch target address
    bool br_taken;          // Actual branch behavior

    // You may introduce other fields as needed

} inst_t;

typedef struct statistics {
    uint64_t instructions_retired;          // Total number of instructions completed/retired
    uint64_t branch_instructions;           // Total branch instructions
    uint64_t correctly_predicted;           // Branches predicted correctly
    double branch_prediction_accuracy;      // Branch prediction accuracy

    uint64_t load_instructions;             // Total load instructions
    uint64_t store_instructions;            // Total store instructions
    uint64_t cache_misses;                  // Total misses in the data cache
    double cache_miss_rate;                 // Cache miss rate

    double average_disp_queue_size;         // Average dispatch queue size
    uint64_t max_disp_queue_size;           // Maximum dispatch queue size
    double average_instructions_retired;    // Average instructions retired per cycle
    uint64_t cycle_count;                   // Total cycle count (runtime)

} proc_stats_t;

bool read_instruction(inst_t* p_inst);

void setup_proc(proc_conf_t *config);
void run_proc(proc_stats_t *stats);
void complete_proc(proc_stats_t *stats);

#endif // PROCSIM_H
