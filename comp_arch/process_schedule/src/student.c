/*
 * student.c
 * Multithreaded OS Simulation for ECE 3056
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"

typedef struct _ll_node {
    pcb_t* process;
    struct _ll_node* next;
} ll_node;

typedef struct _linked_queue {
    ll_node* head;
    ll_node* tail;
} linked_queue;

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);
int queue_push(linked_queue *queue, pcb_t* process);
int queue_ordered_push(linked_queue *queue, pcb_t* process);
pcb_t* queue_pop(linked_queue *queue);


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;
static pthread_mutex_t ready_queue_mutex;
static pthread_cond_t idle_cond;
static linked_queue ready_queue;
char mode = 'f';
unsigned int timeslice;
unsigned int lrtf_cpu_count;


/*
 * pushes a process into the ready queue
 * return 1 on empty ready queue
 * return 0 on adding pcb to a non-empty ready queue
 */
int queue_push(linked_queue *queue, pcb_t* process) {
    ll_node* newNode = (ll_node *)malloc(sizeof(ll_node));
    newNode->process = process;
    newNode->next = NULL;
    if (queue->head == NULL) {
        queue->head = newNode;
        queue->tail = newNode;
        return 1;
    } else {
        queue->tail->next = newNode;
        queue->tail = newNode;
        return 0;
    }
}

int queue_ordered_push(linked_queue * queue, pcb_t* process) {
    ll_node* newNode = (ll_node *)malloc(sizeof(ll_node));
    newNode->process = process;
    newNode->next = NULL;
    unsigned int curr = process->time_remaining;
    ll_node* cursor = NULL;

    if (queue->head == NULL) {
        queue->head = newNode;
        queue->tail = newNode;
    } else {
        cursor = queue->head;
        if (cursor->process->time_remaining < curr) {
            newNode->next = queue->head;
            queue->head = newNode;
        } else {

            while (cursor->next != NULL && cursor->next->process->time_remaining > curr) {   
                cursor = cursor->next;
            }

            if (cursor->next == NULL) {
                cursor->next = newNode;
                queue->tail = newNode;
            } else {
                newNode->next = cursor->next;
                cursor->next = newNode;
            }

        }
    }

    return 0;
}

/* 
 * Removes the first item from the queue and update the queue
 * returns NULL if empty queue
 */
pcb_t* queue_pop(linked_queue *queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    pcb_t * return_process = queue->head->process;
    queue->head = queue->head->next;
    return return_process;
}



/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{

    pthread_mutex_lock(&ready_queue_mutex);

    pcb_t* ready_process = queue_pop(&ready_queue);

    pthread_mutex_unlock(&ready_queue_mutex);

    if (ready_process == NULL) {
        context_switch(cpu_id, NULL, -1);
        return;
    }

    ready_process->state = PROCESS_RUNNING;
    
    if (mode == 'f') {
        context_switch(cpu_id, ready_process, -1);
    } else if (mode == 'l') {
        context_switch(cpu_id, ready_process, (int)ready_process->time_remaining);
    } else if (mode == 'r') {
        context_switch(cpu_id, ready_process, (int)timeslice);
    }

    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = ready_process;
    pthread_mutex_unlock(&current_mutex);
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    pthread_mutex_lock(&ready_queue_mutex);
    while (ready_queue.head == NULL) {
        pthread_cond_wait(&idle_cond, &ready_queue_mutex);
    }
    pthread_mutex_unlock(&ready_queue_mutex);
    schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t* now_running = current[cpu_id];
    now_running->state = PROCESS_READY;
    pthread_mutex_unlock(&current_mutex);

    pthread_mutex_lock(&ready_queue_mutex);
    queue_ordered_push(&ready_queue, now_running);
    pthread_mutex_unlock(&ready_queue_mutex);

    schedule(cpu_id);
}


/*else if (mode == 'r') {
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O    }  request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is LRTF, wake_up() may need
 *      to preempt the CPU with lower remaining time left to allow it to
 *      execute the process which just woke up with higher reimaing time.
 * 	However, if any CPU is currently running idle,
* 	or all of the CPUs are running processes
 *      with a higher remaining time left than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    // Mark as ready
    process->state = PROCESS_READY;
    // Push to ready queue

    if (mode == 'f' || mode == 'r') {
        pthread_mutex_lock(&ready_queue_mutex);
        queue_push(&ready_queue, process);
        pthread_mutex_unlock(&ready_queue_mutex);
    } else if (mode == 'l') {

        pthread_mutex_lock(&ready_queue_mutex);
        queue_ordered_push(&ready_queue, process);
        pthread_mutex_unlock(&ready_queue_mutex);

        pthread_mutex_lock(&current_mutex);
        int force_empt = 1;
        unsigned int shortest_remaining_time = 0;
        unsigned int shortest_cpuid = 0;
        for (unsigned int i = 0; i < lrtf_cpu_count; i++) {
            if (current[i] == NULL) {
                force_empt = 0;
                break; 
            } else {
                // seg faults on reading time remaining
                if (current[i]->time_remaining < shortest_remaining_time || shortest_remaining_time == 0) {
                    shortest_remaining_time = current[i]->time_remaining;
                    shortest_cpuid = i;
                }
            }
        }
        pthread_mutex_unlock(&current_mutex);

        if (force_empt == 1 && shortest_remaining_time < process->time_remaining) {
            force_preempt(shortest_cpuid);
        }

    } else {
        printf("Unknown mode!\n");
    }

    //force_preempt

    pthread_cond_signal(&idle_cond);
    // if LRTF, then ?

}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -l and -r command-line parameters.
 */
int main(int argc, char *argv[])
{
    unsigned int cpu_count;

    /* Parse command-line arguments */
    if (argc != 2 && argc != 3 && argc != 4)
    {
        fprintf(stderr, "ECE 3056 OS Sim -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -l | -r <time slice> ]\n"
            "    Default : FIFO Scheduler\n"
	    "         -l : Longest Remaining Time First Scheduler\n"
            "         -r : Round-Robin Scheduler\n\n");
        return -1;
    }
    cpu_count = strtoul(argv[1], NULL, 0);
    lrtf_cpu_count = cpu_count;

    if (argc == 3 || argc == 4)
    {
        mode = *(argv[2] + 1);
    }

    if (mode == 'r') {
        timeslice = strtoul(argv[3], NULL, 0);
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&ready_queue_mutex, NULL);
    pthread_cond_init(&idle_cond, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);
    
    return 0;
}


