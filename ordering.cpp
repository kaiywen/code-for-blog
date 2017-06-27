/* This file comes from http://preshing.com, a personal blog of Preshing.
 * It is modified to support MacOS
 */

#include <pthread.h>
#include <stdio.h>

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#elif defined(__linux__)
#include <semaphore.h>
#endif

// Set either of these to 1 to prevent CPU reordering
#define USE_CPU_FENCE              0
#define USE_SINGLE_HW_THREAD       0  // Supported on Linux, but not Cygwin or PS3

#define MAX_ITER                   3000000

#if USE_SINGLE_HW_THREAD
#include <sched.h>
#endif

#if defined(__linux__)
#define sem_t sem_t
#define sem_init(x, y, z) sem_init(x,y,z)
#define sem_wait(x) sem_wait(x)
#define sem_post(x) sem_post(x)
#elif defined(__APPLE__)
#define sem_t dispatch_semaphore_t
#define sem_init(x, y, z) do {           \
    *(x) = dispatch_semaphore_create(z);  \
} while (0);
#define sem_wait(x) dispatch_semaphore_wait(*(x), DISPATCH_TIME_FOREVER);
#define sem_post(x) dispatch_semaphore_signal(*(x))
#define sem_destroy(x) dispatch_release(x)
#endif

//-------------------------------------
//  MersenneTwister
//  A thread-safe random number generator with good randomness
//  in a small number of instructions. We'll use it to introduce
//  random timing delays.
//-------------------------------------
#define MT_IA  397
#define MT_LEN 624

class MersenneTwister {
    unsigned int m_buffer[MT_LEN];
    int m_index;

  public:
    MersenneTwister(unsigned int seed);
    // Declare noinline so that the function call acts as a compiler barrier:
    unsigned int integer() __attribute__((noinline));
};

MersenneTwister::MersenneTwister(unsigned int seed) {
    // Initialize by filling with the seed, then iterating
    // the algorithm a bunch of times to shuffle things up.
    for (int i = 0; i < MT_LEN; i++)
        m_buffer[i] = seed;
    m_index = 0;
    for (int i = 0; i < MT_LEN * 100; i++)
        integer();
}

unsigned int MersenneTwister::integer() {
    // Indices
    int i = m_index;
    int i2 = m_index + 1; if (i2 >= MT_LEN) i2 = 0; // wrap-around
    int j = m_index + MT_IA; if (j >= MT_LEN) j -= MT_LEN; // wrap-around

    // Twist
    unsigned int s = (m_buffer[i] & 0x80000000) | (m_buffer[i2] & 0x7fffffff);
    unsigned int r = m_buffer[j] ^ (s >> 1) ^ ((s & 1) * 0x9908B0DF);
    m_buffer[m_index] = r;
    m_index = i2;

    // Swizzle
    r ^= (r >> 11);
    r ^= (r << 7) & 0x9d2c5680UL;
    r ^= (r << 15) & 0xefc60000UL;
    r ^= (r >> 18);
    return r;
}


//-------------------------------------
//  Main program, as decribed in the post
//-------------------------------------

sem_t beginSema1;
sem_t beginSema2;
sem_t endSema;


int X, Y;
int r1, r2;

void *thread1Func(void *param) {
    MersenneTwister random(1);
    for (;;) {
        sem_wait(&beginSema1);
        while (random.integer() % 8 != 0) {}  // Random delay

        // ----- THE TRANSACTION! -----
        X = 1;
#if USE_CPU_FENCE
        asm volatile("mfence" ::: "memory");  // Prevent CPU reordering
#else
        asm volatile("" ::: "memory");  // Prevent compiler reordering
#endif
        r1 = Y;

        sem_post(&endSema);
    }
    return NULL;  // Never returns
};

void *thread2Func(void *param) {
    MersenneTwister random(2);
    for (;;) {
        sem_wait(&beginSema2);
        while (random.integer() % 8 != 0) {}  // Random delay

        // ----- THE TRANSACTION! -----
        Y = 1;
#if USE_CPU_FENCE
        asm volatile("mfence" ::: "memory");  // Prevent CPU reordering
#else
        asm volatile("" ::: "memory");  // Prevent compiler reordering
#endif
        r2 = X;

        sem_post(&endSema);
    }
    return NULL;  // Never returns
};

int main() {
    // Initialize the semaphores
    sem_init(&beginSema1, 0, 0);
    sem_init(&beginSema2, 0, 0);
    sem_init(&endSema, 0, 0);

    // Spawn the threads
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread1Func, NULL);
    pthread_create(&thread2, NULL, thread2Func, NULL);

#if (USE_SINGLE_HW_THREAD) && defined(__linux__)
    // Force thread affinities to the same cpu core.
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    pthread_setaffinity_np(thread1, sizeof(cpu_set_t), &cpus);
    pthread_setaffinity_np(thread2, sizeof(cpu_set_t), &cpus);
#endif

    // Repeat the experiment ad infinitum
    int detected = 0;
    for (int iterations = 1; iterations < MAX_ITER ; iterations++) {
        // Reset X and Y
        X = 0;
        Y = 0;
        // Signal both threads
        sem_post(&beginSema1);
        sem_post(&beginSema2);
        // Wait for both threads
        sem_wait(&endSema);
        sem_wait(&endSema);
        // Check if there was a simultaneous reorder
        if (r1 == 0 && r2 == 0) {
            detected++;
            printf("%d reorders detected after %d iterations\n", detected, iterations);
        }
    }

#if defined(__APPLE__)
    sem_destroy(beginSema1);
    sem_destroy(beginSema2);
    sem_destroy(endSema);
#endif
    return 0;  // Never returns
}

