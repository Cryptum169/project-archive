#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "procsim.h"

// #include <time.h>

typedef struct cache_block {
    uint64_t valid;
    unsigned long long tag;
    uint64_t dirty;
} cache_block;

typedef struct functional_unit{
    int busy;
    int load;
    int index;
    uint64_t addr;
} fu;

typedef struct schedule_queue_row{
    int opcode;
    int dest_reg;
    int dest_reg_ready;
    uint64_t dest_reg_tag;
    int src_reg[2];
    int src_reg_ready[2];
    uint64_t src_reg_tag[2];
    int occupied;
    inst_t instr;
    int fired;
    int fu;
    int remaining_time;
    bool ls_conflict_end;
} sq;

typedef struct register_file {
    uint64_t tag;
    uint64_t ready;
} rf;

typedef struct result_row {
    uint64_t tag;
    uint64_t fetch;
    uint64_t dispatch;
    uint64_t schedule;
    uint64_t execute;
    uint64_t update;
} result_t;

cache_block* l1_cache;
inst_t* dispatch_queue;
int dispatch_queue_head;
int dispatch_queue_tail;
int dispatch_queue_size;
int dq_data_structure_size;
sq* schedule_queue;
uint64_t inst_cnt_sq;
int* branch_table;
// int* score_board;
int schedule_queue_size;
rf register_file[32];
fu* ls_functional_unit;
result_t * result;
uint64_t result_length;
uint64_t result_idx;
uint64_t ghr;
int ghr_length;

uint64_t dispatch_queue_cumulative_size = 0;

int fetch_count;
static uint64_t instr_tag;
int fetched_count;
int cache_size;
int block_size = 6;

int add_unit_count;
int add_unit;
int mul_unit_count;
int mul_unit;
int ls_unit_count;
int ls_unit;

bool branch_started = false;
bool branch_finished = false;
int * finished_fu;

uint64_t result_tag = 0;

int temp_iterator = 0;


/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 *
 * param config Pointer to the run configuration structure
 */
void setup_proc(proc_conf_t *config)
{
    instr_tag = 0;
    cache_size = config->c;


    l1_cache = (cache_block *)malloc(sizeof(cache_block) * (long unsigned int) (1 << (cache_size - block_size)));
    for (int i = 0; i < (1 << (cache_size - block_size)); i++) {
        l1_cache[i].valid = 0;
        l1_cache[i].tag = 0;
        l1_cache[i].dirty = 0;
    }

    // Initialize Branch Predictor
    int entry_table_size = config->g;
    ghr = 0;
    ghr_length = entry_table_size;
    branch_table = (int*)malloc(sizeof(int) * (long unsigned int)(1 << entry_table_size));
    for (int i = 0; i < (1 << entry_table_size); i++) {
        branch_table[i] = 1;
    }

    // Initialize Dispatch Queue
    fetch_count = config->f;
    dq_data_structure_size = 2000000;
    dispatch_queue = (inst_t*)malloc(sizeof(inst_t) * dq_data_structure_size);
    dispatch_queue_head = 0;
    dispatch_queue_tail = 0;
    dispatch_queue_size = 0;

    result_length = 2000000;
    result_idx = 0;
    result = (result_t*)malloc(sizeof(result_t) * (long unsigned int) result_length);

    // Initialize Reservation Queue
    schedule_queue_size = (config->k0 + config->k1 + config->k2)* config->r;
    inst_cnt_sq = 0;
    finished_fu = (int*)malloc(sizeof(int) * schedule_queue_size);
    schedule_queue = (sq*)malloc(sizeof(sq) * schedule_queue_size);

    for (int i = 0; i < schedule_queue_size; i++) {
        finished_fu[i] = -1;
        schedule_queue[i].opcode = 1;
        schedule_queue[i].dest_reg = -1;
        schedule_queue[i].src_reg[0] = -1;
        schedule_queue[i].src_reg[1] = -1;

        schedule_queue[i].src_reg_ready[0] = 0;
        schedule_queue[i].src_reg_ready[1] = 0;

        schedule_queue[i].src_reg_tag[0] = 0;
        schedule_queue[i].src_reg_tag[1] = 0;

        schedule_queue[i].dest_reg_tag = 0;
        schedule_queue[i].dest_reg_ready = 1;

        schedule_queue[i].occupied = 0;
        schedule_queue[i].fired = 0;
        schedule_queue[i].remaining_time = 0;
        schedule_queue[i].instr.schedule_first = false;
        schedule_queue[i].instr.execute_first = false;
        schedule_queue[i].ls_conflict_end = false;

        inst_t placeholder;
        placeholder.inst_addr = 0;
        placeholder.opcode = 0;
        placeholder.sequential_tag = 0;
        placeholder.ld_st_addr = 0;
        schedule_queue[i].instr = placeholder;
    }
    
    for (int i = 0; i < 32; i++) {
        register_file[i].tag = 0;
        register_file[i].ready = 1;
    }

    // Initialize Functional Units
    add_unit_count = config->k0;
    add_unit = add_unit_count;
    mul_unit_count = config->k1;
    mul_unit = mul_unit_count;
    ls_unit_count = config->k2;
    ls_unit = ls_unit_count;

    ls_functional_unit = (fu*)malloc(sizeof(fu) * (long unsigned int) ls_unit_count);
    
    for (int i = 0; i < ls_unit_count; i++) {
        ls_functional_unit[i].busy = 0;
        ls_functional_unit[i].load = 0;
        ls_functional_unit[i].addr = 0;
    }

    return;
}

/**
 * Subroutine that simulates the processor. The processor should fetch instructions as
 * appropriate, until all instructions have executed
 *
 * param p_stats Pointer to the statistics structure
 */
int cycle = 0;
void run_proc(proc_stats_t* p_stats)
{
    // fetched_count = 1;
    bool continue_bool = true;
    bool continue_fetch =  true;
    while (continue_bool) {
        p_stats->cycle_count++;
        update_state(p_stats);
        execute_instr(p_stats);
        schedule_instr();
        dispatch_instr();
        if (continue_fetch) {
            continue_fetch = fetch_instr(p_stats) != 0;
        }

        update_state_2();
        cycle++;

        dispatch_queue_cumulative_size += dispatch_queue_size;
        continue_bool = !((inst_cnt_sq == 0 && (dispatch_queue_head == dispatch_queue_tail))) || continue_fetch;

    }

    p_stats->cycle_count--;
    return;
}

int compare (const void * a, const void * b)
{
    result_t *result_A = (result_t *)a;
    result_t *result_B = (result_t *)b;
    return (result_A->tag - result_B->tag);
}

/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc. Make sure to free all heap memory!
 *
 * param p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats)
{
    qsort(result, result_idx, sizeof(result_t), compare);

    printf("FETCH	DISP	SCHED	EXEC	SUPDATE\n");
    for (uint64_t i = 0; i < result_idx; i++) {
        printf("%"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64"\n", result[i].fetch,
            result[i].dispatch, result[i].schedule,result[i].execute,result[i].update);
    }

    p_stats->branch_prediction_accuracy = (double) p_stats->correctly_predicted / (double) p_stats->branch_instructions;
    p_stats->cache_miss_rate = (double) p_stats->cache_misses / (double)(p_stats->load_instructions + p_stats->store_instructions);
    p_stats->average_instructions_retired = (double) p_stats->instructions_retired / (double) p_stats->cycle_count;
    p_stats->average_disp_queue_size = (double) dispatch_queue_cumulative_size / (double)p_stats->cycle_count;

    free(l1_cache);
    free(branch_table);
    free(dispatch_queue);
    free(result);
    free(finished_fu);
    free(schedule_queue);
    free(ls_functional_unit);
    return;
}

bool prepare_to_stall = false;
int fetch_instr(proc_stats_t *stats) {
    // Tail always point to the Free space!
    bool fetch_result;
    int fetched_instruction_count = 0;
    for (int iteration = 0; iteration < fetch_count; iteration++) {
        // One Cycle
        fetch_result = read_instruction(&dispatch_queue[dispatch_queue_tail]);
        if (!fetch_result) {
            return 0;
        }

        if (dispatch_queue[dispatch_queue_tail].opcode ==  4) {
            stats->load_instructions++;
        } else if (dispatch_queue[dispatch_queue_tail].opcode == 5) {
            stats->store_instructions++;
        }

        dispatch_queue[dispatch_queue_tail].fetch = cycle;
        dispatch_queue[dispatch_queue_tail].sequential_tag = result_tag;
        result_tag++;
        dispatch_queue[dispatch_queue_tail].execute_first = true;
        dispatch_queue[dispatch_queue_tail].schedule_first = true;
        dispatch_queue[dispatch_queue_tail].dispatch_first = true;

        if (dispatch_queue[dispatch_queue_tail].opcode == 6) {
            stats->branch_instructions++;
            // branch
            int ghr_idx = ghr & ((1 << ghr_length) - 1);
            int btb_idx = ((dispatch_queue[dispatch_queue_tail].inst_addr >> 2) % (1 << ghr_length)) ^ ghr_idx;
            bool predicted_behavior = (branch_table[btb_idx] - 2) > -1;
            dispatch_queue[dispatch_queue_tail].predicted_br = predicted_behavior;

            if (dispatch_queue[dispatch_queue_tail].br_taken == predicted_behavior) {
                stats->correctly_predicted++;
            }
        }

        fetched_instruction_count++;
        dispatch_queue_tail++;
        dispatch_queue_size++;

        if ((uint64_t) dispatch_queue_size > stats->max_disp_queue_size) {
            stats->max_disp_queue_size = dispatch_queue_size;
        }
    }
    return 1;
}

void dispatch_instr() {
    // If space in stuff
    // check if there's space
    // clock_t start = clock();
    int dispatched_instruction_count = 0;

    while (temp_iterator < dispatch_queue_tail) {
        if (dispatch_queue[temp_iterator].dispatch_first) {
            dispatch_queue[temp_iterator].dispatch_first = false;
            dispatch_queue[temp_iterator].dispatch = cycle;
        }
        temp_iterator++;
    }

    if (branch_started) {
        return;
    }

    bool dispatch_queue_not_end = true;
    
    for (int sq_iter = 0; sq_iter < schedule_queue_size; sq_iter++) {
        
        if (dispatch_queue_head == dispatch_queue_tail || !dispatch_queue_not_end) {
            break;
        }


        if (schedule_queue[sq_iter].occupied == 0 && dispatch_queue_not_end) {
            dispatched_instruction_count++;
            // If empty in schedule queue 
            inst_t new_instruction = dispatch_queue[dispatch_queue_head];

            // Account for NOP

            while (new_instruction.opcode == 1) {
                dispatch_queue_head++;
                dispatch_queue_size--;

                if (dispatch_queue_size == 0) {
                    dispatch_queue_not_end = false;
                    break;
                }

                new_instruction = dispatch_queue[dispatch_queue_head];
            }

            if (!dispatch_queue_not_end) {
                break;
            }

            if (new_instruction.opcode == 1) {
                printf("what the fuck?\n");
                dispatch_queue_not_end = false;
                break;
            }

            dispatch_queue_head++;
            dispatch_queue_size--;

            schedule_queue[sq_iter].occupied = 1;
            inst_cnt_sq++;
            schedule_queue[sq_iter].opcode = new_instruction.opcode;
            schedule_queue[sq_iter].dest_reg_tag = instr_tag;
            instr_tag++;

            schedule_queue[sq_iter].dest_reg = new_instruction.dest_reg;
            schedule_queue[sq_iter].instr = new_instruction;

            for (int src_iter = 0; src_iter < 2; src_iter++) {
                schedule_queue[sq_iter].src_reg[src_iter] = new_instruction.src_reg[src_iter];
                if (schedule_queue[sq_iter].src_reg[src_iter] == -1) {
                    schedule_queue[sq_iter].src_reg_ready[src_iter] = 1;
                    continue;
                }

                if (register_file[new_instruction.src_reg[src_iter]].ready == 1) {
                    // Register ready
                    schedule_queue[sq_iter].src_reg_ready[src_iter] = 1;
                } else {
                    schedule_queue[sq_iter].src_reg_tag[src_iter] = register_file[new_instruction.src_reg[src_iter]].tag;
                    schedule_queue[sq_iter].src_reg_ready[src_iter] = 0;
                }
            }

            if (schedule_queue[sq_iter].dest_reg != -1) {
                register_file[schedule_queue[sq_iter].dest_reg].tag = schedule_queue[sq_iter].dest_reg_tag;
                register_file[schedule_queue[sq_iter].dest_reg].ready = 0;
            }

            if (schedule_queue[sq_iter].opcode == 6) {                
                if (schedule_queue[sq_iter].instr.br_taken != schedule_queue[sq_iter].instr.predicted_br) {
                    branch_started = true;
                    branch_finished = false;
                    break;
                }
            }

        }
    }
}
void schedule_instr() {
    int opcode;
    int num_scheduled = 0;
    int sche_mul_unit = 1;
    for (int sq_iter = 0; sq_iter < schedule_queue_size; sq_iter++) {

        if (schedule_queue[sq_iter].occupied == 1 && schedule_queue[sq_iter].fired == 0) {

            if (schedule_queue[sq_iter].instr.schedule_first) {
                schedule_queue[sq_iter].instr.schedule = cycle;
                schedule_queue[sq_iter].instr.schedule_first = false;
            }

            opcode = schedule_queue[sq_iter].opcode;

            if (opcode == 4 || 5) {
                schedule_queue[sq_iter].ls_conflict_end = true;
            }


            if (schedule_queue[sq_iter].src_reg_ready[0] == 1 && schedule_queue[sq_iter].src_reg_ready[1] == 1) {
                

                if (opcode == 2 || opcode == 6) {

                    if (add_unit > 0) {
                        add_unit--;
                        schedule_queue[sq_iter].fired = 1;
                        schedule_queue[sq_iter].remaining_time = 1;
                        num_scheduled++;
                    }
                } else if (opcode == 3) {
                    if (sche_mul_unit > 0) {
                        sche_mul_unit--;
                        schedule_queue[sq_iter].fired = 1;
                        schedule_queue[sq_iter].remaining_time = 3;
                        num_scheduled++;
                        // schedule_queue[sq_iter].instr.schedule = cycle;
                    }
                } else if (opcode == 4 || opcode == 5) {
                    bool cache_opeable = true;
                    uint64_t addr_value = schedule_queue[sq_iter].instr.ld_st_addr;
                    int index_value = (int)(addr_value >> (block_size)) & ((1 << (cache_size - block_size)) - 1);

                    for (int ls_unit_iter = 0; ls_unit_iter < ls_unit_count; ls_unit_iter++) {
                        if (ls_functional_unit[ls_unit_iter].busy == 0) {
                            continue;
                        }

                        if (opcode == 5) {
                            cache_opeable = cache_opeable && !(ls_functional_unit[ls_unit_iter].addr == addr_value);
                        } else {
                            bool old_store = ls_functional_unit[ls_unit_iter].load != opcode;
                            cache_opeable = cache_opeable && !(ls_functional_unit[ls_unit_iter].addr == addr_value && old_store);
                        }

                        // No same index
                        cache_opeable = cache_opeable && !(ls_functional_unit[ls_unit_iter].index == index_value);
                    }

                    for (int i = 0; i < schedule_queue_size; i++) {
                        if (schedule_queue[i].occupied == 1 && schedule_queue[i].instr.ld_st_addr == addr_value && schedule_queue[i].ls_conflict_end == true) { // && ls_functional_unit[schedule_queue[i].fu].busy == 1) {
                            if (schedule_queue[i].instr.sequential_tag < schedule_queue[sq_iter].instr.sequential_tag) {
                                if (opcode == 4 && schedule_queue[i].opcode == 5) {
                                    cache_opeable = false;
                                    break;
                                } else if (opcode == 5) {
                                    cache_opeable = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (ls_unit > 0 && cache_opeable) {
                        ls_unit--;
                        schedule_queue[sq_iter].remaining_time = 11;
                        num_scheduled++;

                        for (int ls_unit_iter = 0; ls_unit_iter < ls_unit_count; ls_unit_iter++) {
                            if (ls_functional_unit[ls_unit_iter].busy == 1) {
                                continue;
                            }

                            ls_functional_unit[ls_unit_iter].busy = 1;
                            ls_functional_unit[ls_unit_iter].addr = addr_value;
                            ls_functional_unit[ls_unit_iter].load = opcode;
                            ls_functional_unit[ls_unit_iter].index = index_value;
                            schedule_queue[sq_iter].fu = ls_unit_iter;
                            schedule_queue[sq_iter].fired = 1;
                            schedule_queue[sq_iter].ls_conflict_end = true;

                            break;
                        }
                    }
                }
            }
        }
    }
}

void execute_instr(proc_stats_t *stats) {
    int executed = 0;
    uint64_t addr;
    char type_w;
    int cache_result;
    for (int sq_iter = 0; sq_iter < schedule_queue_size; sq_iter++) {
        if (schedule_queue[sq_iter].occupied == 1) {
            if (schedule_queue[sq_iter].fired == 1) {
                if (schedule_queue[sq_iter].instr.execute_first) {
                    schedule_queue[sq_iter].instr.execute = cycle;
                    schedule_queue[sq_iter].instr.execute_first = false;
                }

                if (schedule_queue[sq_iter].remaining_time == 11) {
                    addr = schedule_queue[sq_iter].instr.ld_st_addr;
                    type_w = schedule_queue[sq_iter].opcode == 4 ? 'R' : 'W';
                    cache_result = cache_access(addr, type_w, stats);
                    if (cache_result == 1) {
                        schedule_queue[sq_iter].remaining_time = 1;
                    } else {
                        schedule_queue[sq_iter].remaining_time = 10;
                    }
                }
                schedule_queue[sq_iter].remaining_time--;
                executed++;
                
                if (schedule_queue[sq_iter].remaining_time == 0) {
                    int opcode = schedule_queue[sq_iter].opcode;
                    if (opcode == 2 || opcode == 6) {
                        add_unit++;
                    // } else if (opcode == 3) {
                    } else if (opcode == 4 || opcode == 5) {
                        ls_unit++;
                        ls_functional_unit[schedule_queue[sq_iter].fu].busy = 0;
                    }
                }
            }

        }
    }
}


int compare_sq(const void * a, const void * b) {
    sq* resultA = (sq*)a;
    sq* resultB = (sq*)b;
    return (resultA->instr.sequential_tag  * resultB->occupied - resultB->instr.sequential_tag * resultA->occupied);
}

void update_state_2() {

    for (int sq_iter = 0; sq_iter < schedule_queue_size; sq_iter++) {
        if (finished_fu[sq_iter] != -1) {
            schedule_queue[finished_fu[sq_iter]].occupied = 0;
            inst_cnt_sq--;
            schedule_queue[finished_fu[sq_iter]].fired = 0;
            finished_fu[sq_iter] = -1;
        }
    }
    qsort(schedule_queue, schedule_queue_size, sizeof(sq), compare_sq);

}

void print_binary(int n) {
    while (n) {
        if (n & 1) {
            printf("1");
        } else {
            printf("0");
        }
        n >>= 1;
    }
    printf("\n");
}

void update_state(proc_stats_t *stats) {
    int opcode = 0;
    int dest_reg;
    int retired = 0;
    uint64_t dest_reg_tag;
    int finished_fu_idx = 0;

    // qsort(schedule_queue, schedule_queue_size, sizeof(sq), compare_sq);

    for (int sq_iter = 0; sq_iter < schedule_queue_size; sq_iter++) {
        if (schedule_queue[sq_iter].occupied == 1) {
            if (schedule_queue[sq_iter].fired == 1) {
                if (schedule_queue[sq_iter].remaining_time == 0) {
                    // Finished
                    schedule_queue[sq_iter].instr.update = cycle;
                    schedule_queue[sq_iter].ls_conflict_end = false;

                    finished_fu[finished_fu_idx] = sq_iter;
                    finished_fu_idx++;
                    stats->instructions_retired++;
                    retired++;
                    opcode = schedule_queue[sq_iter].opcode;
                    if (opcode == 6) {
                        if (schedule_queue[sq_iter].instr.br_taken != schedule_queue[sq_iter].instr.predicted_br) {
                            branch_started = false;
                            branch_finished = true;
                            prepare_to_stall = false;
                        }

                        int ghr_idx = ghr & ((1 << ghr_length) - 1);
                        int btb_idx = ((schedule_queue[sq_iter].instr.inst_addr >> 2) % (1 << ghr_length)) ^ ghr_idx;

                        // printf("Cycle %d Access Index: %d\n", cycle, btb_idx);
                        // printf("Counter value %d -> ", branch_table[btb_idx]);
                        
                        if (schedule_queue[sq_iter].instr.br_taken) {
                            branch_table[btb_idx]++;
                        } else {
                            branch_table[btb_idx]--;
                        }

                        if (branch_table[btb_idx] > 3) {
                            branch_table[btb_idx] = 3;
                        }

                        if (branch_table[btb_idx] < 0) {
                            branch_table[btb_idx] = 0;   
                        }

                        // printf("%d\n", branch_table[btb_idx]);

                        ghr = ghr << 1;
                        if (schedule_queue[sq_iter].instr.br_taken) {
                            ghr = ghr | 1;
                            // printf("Branch taken\n");
                        } else {
                            // printf("Branch not taken\n");
                        }

                    }

                    if (result_idx < result_length) {
                        result[result_idx].tag = schedule_queue[sq_iter].instr.sequential_tag;
                        result[result_idx].fetch = schedule_queue[sq_iter].instr.fetch;
                        result[result_idx].dispatch = schedule_queue[sq_iter].instr.dispatch;
                        result[result_idx].schedule = schedule_queue[sq_iter].instr.schedule;
                        result[result_idx].execute = schedule_queue[sq_iter].instr.execute;
                        result[result_idx].update = schedule_queue[sq_iter].instr.update;
                        result_idx++;
                    } else {
                        uint64_t new_result_length = result_length * 2;
                        result_t* new_result = (result_t*)malloc(sizeof(result_t) * new_result_length);
                        for (uint64_t result_iter = 0; result_iter < result_length; result_iter++) {
                            new_result[result_iter] = result[result_iter];
                        }
                        free(result);
                        result_length = new_result_length;
                        result = new_result;
                        result[result_idx].tag = schedule_queue[sq_iter].instr.sequential_tag;
                        result[result_idx].fetch = schedule_queue[sq_iter].instr.fetch;
                        result[result_idx].dispatch = schedule_queue[sq_iter].instr.dispatch;
                        result[result_idx].schedule = schedule_queue[sq_iter].instr.schedule;
                        result[result_idx].execute = schedule_queue[sq_iter].instr.execute;
                        result[result_idx].update = schedule_queue[sq_iter].instr.update;
                        result_idx++;
                    }


                    dest_reg = schedule_queue[sq_iter].dest_reg;
                    if (!(dest_reg == -1)) {
                        dest_reg_tag = schedule_queue[sq_iter].dest_reg_tag;
                        // printf("Freeing related dest_reg %d, tag %ld\n", dest_reg, dest_reg_tag);
                        for (int sec_ser = 0; sec_ser < schedule_queue_size; sec_ser++) {
                            if (schedule_queue[sec_ser].occupied == 1 &&
                                schedule_queue[sec_ser].src_reg_tag[0] == dest_reg_tag &&
                                schedule_queue[sec_ser].src_reg[0] == dest_reg) {
                                    schedule_queue[sec_ser].src_reg_ready[0] = 1;
                            }
                            
                            if (schedule_queue[sec_ser].occupied == 1 &&
                                schedule_queue[sec_ser].src_reg_tag[1] == dest_reg_tag &&
                                schedule_queue[sec_ser].src_reg[1] == dest_reg) {
                                    schedule_queue[sec_ser].src_reg_ready[1] = 1;
                            }
                        }

                        if (register_file[dest_reg].tag == dest_reg_tag) {
                            register_file[dest_reg].ready = 1;
                        }
                    }
                }
            }
        }
    }
}

int cache_access(uint64_t addr, char w, proc_stats_t *stats) {
    // return 1 for hit, 0 for miss
    unsigned long long tag_value = addr >> cache_size;
    int index_value = (int)(addr >> (block_size)) & ((1 << (cache_size - block_size)) - 1);

    if (l1_cache[index_value].tag == tag_value && l1_cache[index_value].valid == 1) {
        // hit
        if (w == 'W') {
            l1_cache[index_value].dirty = 1;
        }
        return 1;

    } else {
        l1_cache[index_value].tag = tag_value;
        l1_cache[index_value].valid = 1;
        
        if (w == 'W') {
            l1_cache[index_value].dirty = 1;
        } else {
            l1_cache[index_value].dirty = 0;
        }

        stats->cache_misses++;

        return 0;
    }
}

