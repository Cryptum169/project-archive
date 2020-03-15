#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cachesim.h"

// Include math to include Log
#include <math.h>

// Least Recently Used replacement
counter_t accesses = 0, write_accesses = 0;
counter_t read_hits = 0, write_hits = 0, writebacks = 0;

// counter_t access_misses = 0, checkHit = 0;

// A row of cache
typedef struct cache_row{
	int valid;
	int tag;
	int dirty;
}cache_row;

// Function Prototype
void update_cachesim_lru_access(int, int);
int lru_stack_kickout(int, int);

// Cache and LRU stack initialization
cache_row* cache;
int* lru_stack;

// Parameters 
int block_length;
int index_length;
int tag_length;
int ways_count;
int cache_count;
int block_size;

void cachesim_init(int blocksize, int cachesize, int ways) {
	// Calculate index parameters
	int cache_line_count = cachesize / blocksize / ways;
	block_length = log(blocksize) / log(2);
	index_length = log(cache_line_count) / log(2);
	tag_length = 32 - block_length - index_length;

	// Allocate memory space for simulator
	// 2D dynamic cache_row struct array for each memory block
	cache = (cache_row *)malloc(sizeof(cache_row) * cache_line_count * ways); 
	// 2D dynamic int array for LRU
	lru_stack = (int *)malloc(sizeof(int) * cache_line_count * ways); 

	for (int i = 0; i < cache_line_count * ways; i++)
	{
		// Initialize parameters
		cache[i].valid = 0;
		cache[i].tag = -1;
		cache[i].dirty = 0;
		lru_stack[i] = i % ways;
	}

	// Store variables for global reference
	ways_count = ways;
	cache_count = cache_line_count;
	block_size = blocksize;
}


void cachesim_access(addr_t physical_addr, int write) {
	// write == 1 means write, else read
	// Update access count
	accesses++;
	if (write == 1) {
		write_accesses++;
	}

	// Calculate tag
	int tag_value = physical_addr >> (block_length + index_length);
	// Use AND operation to eliminate tag value on higher bits
	int index_value = (physical_addr >> (block_length)) & ((1 << index_length) - 1);


	for (int i = 0; i < ways_count; i++) {
		// Calcualte Location of access in 2D cache array
		int mem_location = index_value * ways_count + i;

		// If valid tag hit
		if (cache[mem_location].tag == tag_value && cache[mem_location].valid == 1)
		{
			if (write == 1)
			{
				// write operation
				write_hits++;
				cache[mem_location].dirty = 1;
			}
			else
			{
				// read operation
				read_hits++;
			}
			// Update LRU Stack
			update_cachesim_lru_access(index_value, i);
			// Exit traversal as cache operation has completed
			i = ways_count + 5;
		}
		else if (cache[mem_location].valid == 0)
		{
			// if vacancy in cache slot, pass in memory
			cache[mem_location].tag = tag_value;
			cache[mem_location].valid = 1;
			if (write == 1) {
				cache[mem_location].dirty = 1;
			} else {
				cache[mem_location].dirty = 0;
			}
			// Update LRU Stack
			update_cachesim_lru_access(index_value, i);
			// Exit traversal as cache operation has completed
			i = ways_count + 5;
		} else if (i == ways_count - 1) {
			// If no cache hit and invalid vacancy

			// Calculate the location to write to, based on kicked out 
			mem_location = mem_location - i + lru_stack_kickout(index_value, write);
			cache[mem_location].tag = tag_value;
			cache[mem_location].valid = 1;
			cache[mem_location].dirty = 0;
			// access_misses++;
		}
	}
}


void cachesim_print_stats() {
	// printf("%llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu \n", accesses, accesses - write_accesses, write_accesses, write_hits + read_hits, read_hits, write_hits, writebacks * block_size, (accesses - write_hits - read_hits) * block_size);
	printf("%llu, %llu, %llu, %llu\n", accesses, read_hits + write_hits, accesses - (read_hits + write_hits), writebacks);
}

void update_cachesim_lru_access(int index_value, int access_location) {
	// Right is most recently used, left is least recently used
	int location = -1;
	int mem_location = index_value * ways_count; 

	// Find the set that is been accessed
	for (int j = 0; j < ways_count; j++) {
		if (lru_stack[mem_location + j] == access_location) {
			location = j;
			j = ways_count + 5;
		}
	}

	// Shift array leftwards
	for (int j = location; j < ways_count - 1; j++) {
		lru_stack[mem_location + j] = lru_stack[mem_location + j + 1];
	}

	// Put most recently acccessed on the right most location
	lru_stack[mem_location + ways_count - 1] = access_location;
}


int lru_stack_kickout(int index_value, int write) {
	int mem_location = index_value * ways_count;
	int k = 0;
	// Determine if writeback is needed
	if (cache[mem_location + lru_stack[mem_location]].dirty != 0) {
		writebacks++;
		// k = 1;
	}

	if (k == 1)
	{
		printf("Prev, %d, %d, %d, %d, %d, %d, %d, %d\n", lru_stack[mem_location], lru_stack[mem_location + 1], lru_stack[mem_location + 2], lru_stack[mem_location + 3], lru_stack[mem_location + 4], lru_stack[mem_location + 5], lru_stack[mem_location + 6], lru_stack[mem_location + 7]);
	}

	// Since left is LRU, right is MRU, store the index stored in LRU
	int returnVal = lru_stack[mem_location];

	// Update LRU stack since least recently used index has been accessed
	for (int i = 0; i < ways_count - 1; i++) {
		lru_stack[mem_location + i] = lru_stack[mem_location + i + 1];
	}
	lru_stack[mem_location + ways_count - 1] = returnVal;

	if (k == 1) {
		printf("Next, %d, %d, %d, %d, %d, %d, %d, %d\n", lru_stack[mem_location], lru_stack[mem_location + 1], lru_stack[mem_location + 2], lru_stack[mem_location + 3], lru_stack[mem_location + 4], lru_stack[mem_location + 5], lru_stack[mem_location + 6], lru_stack[mem_location + 7]);
		printf("returnVal: %d\n",returnVal);
	}

	return returnVal;
}