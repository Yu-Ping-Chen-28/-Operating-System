#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <assert.h>

//for getopt (parse command line arguments)  
int n = -1;
float t = -1.0;
char *s = NULL;
char *p = NULL;
//for barrier
pthread_barrier_t barrier;
//for mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//for busy waiting
double time_wait;
//stored thread info
typedef struct {
    int id;
    int policy;
    int priority;
} ThreadInfo;
//get the CPU time of the current thread in seconds function
static double my_clock(void) {
    struct timespec t;
    // ensure clock_gettime can excute successfully
    assert(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t) == 0);
    //calculate total seconds
    double seconds = t.tv_sec + 1e-9 * t.tv_nsec;
    return seconds;
}
//worker thread function
void *thread_func(void *arg) {
    /* 1. Wait until all threads are ready */
        pthread_barrier_wait(&barrier);        
    /* 2. Do the task */
    for (int i = 0; i < 3; i++) {
        //the usage of a mutex to control critical section
        pthread_mutex_lock(&mutex);
        {
             printf("Thread %d is running\n", *((int *)arg));
             /* Busy for <time_wait> seconds */
                double sttime = my_clock();
                while (1) {
                    if (my_clock() - sttime > 1* time_wait)
                        break;
            }
        }
        pthread_mutex_unlock(&mutex);
        
    }
    /* 3. Exit the function */
    pthread_exit(NULL);
}

//main function
int main(int argc, char **argv) {
    /* 1. Parse program arguments */
    int opt;
    //processes command-line options. use getopt function
    //n: num_threads(integer), t: time_wait(float), s: policies, p: priorities
    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 't':
                t = atof(optarg);
                break;
            case 's':
                s = optarg;
                break;
            case 'p':
                p = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -n <num_threads> -t <time_wait> -s <policies> -p <priorities>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // copy s(policies), p(priority) to policy_input, priority_input
    char *policy_input = strdup(s);
    char *priority_input = strdup(p);
    char policy_array[n][10];
    char priority_array[n][10];
    int policy[n];
    int priority[n];

    int id = 0;
    // copy policy and prioirt to  policy_array and priority_array, and "," is the delimiter
    char *policy_tokens = strtok(policy_input, ",");
    strcpy(policy_array[id++], policy_tokens);

    while ((policy_tokens = strtok(NULL, ",")) != NULL) {
        strcpy(policy_array[id++], policy_tokens);
    }

    id = 0;
    char *priority_tokens = strtok(priority_input, ",");
    strcpy(priority_array[id++], priority_tokens);

    while ((priority_tokens = strtok(NULL, ",")) != NULL) {
        strcpy(priority_array[id++], priority_tokens);
    }

    free(policy_input);
    free(priority_input);

    //set policy's value for SCH_OTHER, SCH_FIFO
    //set priority's value to integer
    for (int i = 0; i < n; i++) {
        if (strcmp(policy_array[i], "NORMAL") == 0) {
            policy[i] = SCHED_OTHER;
        } else if (strcmp(policy_array[i], "FIFO") == 0) {
            policy[i] = SCHED_FIFO;
        } else {
            fprintf(stderr, "Error: Unknown scheduling policy.\n");
            exit(EXIT_FAILURE);
        }
        priority[i] = atoi(priority_array[i]);
    }

    /* 2. Create <num_threads> worker threads */
    //create thread id and thread info
    //malloc memory for tid and threadInfo
    //set time_wait
    pthread_t *tid = (pthread_t *)malloc(sizeof(pthread_t) * n);
    ThreadInfo *threadInfo = (ThreadInfo *)malloc(sizeof(ThreadInfo) * n);
    time_wait = t;

    //set threadInfo's value for n thread
    for (long i = 0; i < n; i++) {
        threadInfo[i].id = i;
        threadInfo[i].policy = policy[i];
        threadInfo[i].priority = priority[i];
    }

    /* 3. Set CPU affinity */
    //the process will be executed on CPU 0
    cpu_set_t set; CPU_ZERO(&set); CPU_SET(0, &set);
    //set CPU affinity for the thread which is calling
    sched_setaffinity(getpid(), sizeof(set), &set);

    /* 4. Set the attributes to each thread */
    //set attr
    pthread_attr_t attr;
    
    //initail barrier
    pthread_barrier_init(&barrier, NULL, n + 1);
    //create n thread
    for (long i = 0; i < n; i++) {
        //initail attr
        pthread_attr_init(&attr);
        //set inheritsched to explicit, ensure the thread's scheduling policy and priority will be set by the attr we set
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        //set scheduling policy and priority
        pthread_attr_setschedpolicy(&attr, threadInfo[i].policy);
        struct sched_param param;
        param.sched_priority = threadInfo[i].priority;
        pthread_attr_setschedparam(&attr, &param);
        //create thread, and pass thread id and attr to thread_func
        int result = pthread_create(&tid[i], &attr, thread_func, (void *)&threadInfo[i].id);
        if (result != 0) {
            fprintf(stderr, "Error creating thread %ld: %s\n", i, strerror(result));
            exit(EXIT_FAILURE);
        }
    }
    //wait until all threads are ready
    pthread_barrier_wait(&barrier);
    
    /* 5. Start all threads at once */
    for (int i = 0; i < n; i++) {
        //join thread
        int result = pthread_join(tid[i], NULL);
        if (result != 0) {
            fprintf(stderr, "Error joining thread %d: %s\n", i, strerror(result));
            exit(EXIT_FAILURE);
        }
    }
    /* 6. Wait for all threads to finish  */
    //destroy barrier
    pthread_barrier_destroy(&barrier);
    free(tid);
    free(threadInfo);

    return 0;
}