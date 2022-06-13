#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"

#define NR_TH_P6 4
#define NR_TH_P2 42
#define NR_TH_P5 5
#define NR_TH_BARRIER 6
#define TARGET_THREAD 14 // in process 2

typedef struct
{
    int idThread;
    int idProcess;
} THStruct;

sem_t s41; // same process synchronisation
sem_t s14;

sem_t* sem51_63; // different processes synchronisation
sem_t* sem63_53;

void* task65(void* arg)
{
    THStruct* s = (THStruct*)arg;
    if(s->idProcess == 5 && s->idThread == 3) // 5.3 waits for 6.3
        sem_wait(sem63_53);
    if(s->idProcess == 6 && s->idThread == 3) // 6.3 waits for 5.1
        sem_wait(sem51_63);

    if(s->idProcess == 6 && s->idThread == 1) // same process
        sem_wait(&s41);

    info(BEGIN, s->idProcess, s->idThread);

    if(s->idProcess == 6 && s->idThread == 4) // same process
    {
        sem_post(&s41);
        sem_wait(&s14);
    }
    
    info(END, s->idProcess, s->idThread);

    if(s->idProcess == 5 && s->idThread == 1) // 5.1 starts so 6.3 is free
        sem_post(sem51_63);
    if(s->idProcess == 6 && s->idThread == 3) // 6.3 starts so 5.3 is free
        sem_post(sem63_53);

    if(s->idThread == 1 && s->idProcess == 6) // same process
        sem_post(&s14);
    return NULL;
}

int resolved_TH_P2 = 0;
int waiting_TH_P2 = NR_TH_P2;
int p2_th14_Ended = 0;
pthread_mutex_t nrThreads = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
sem_t semThreads;

void* task2(void* arg) // thread barrier
{
    THStruct* s = (THStruct*)arg;
    sem_wait(&semThreads);
    info(BEGIN, s->idProcess, s->idThread);
    pthread_mutex_lock(&nrThreads);
    resolved_TH_P2++;
    waiting_TH_P2--;
    pthread_mutex_unlock(&nrThreads);
    if(resolved_TH_P2 == NR_TH_BARRIER)
        pthread_cond_signal(&cond2);
    if(s->idThread == TARGET_THREAD)
    {
        while(resolved_TH_P2 != NR_TH_BARRIER)
            pthread_cond_wait(&cond2, &mutex2);
        p2_th14_Ended = 1;
    }
    if(waiting_TH_P2 <= NR_TH_BARRIER && p2_th14_Ended == 0)
        pthread_mutex_lock(&mutex);
    pthread_mutex_lock(&nrThreads);
    resolved_TH_P2--;
    pthread_mutex_unlock(&nrThreads);
    info(END, s->idProcess, s->idThread);
    if(p2_th14_Ended)
        pthread_mutex_unlock(&mutex);
    sem_post(&semThreads);
    return NULL;
}

int main(){
    init();

    info(BEGIN, 1, 0); // P1
    sem_init(&semThreads, 0, 6); // same process semaphores
    sem_init(&s14, 0, 0);
    sem_init(&s41, 0, 0);
    sem51_63 = sem_open("sem51_63", O_CREAT, 0644, 0); // different processes semaphores
    sem63_53 = sem_open("sem63_53", O_CREAT, 0644, 0);
    
    pid_t pid2 = -1;
    pid2 = fork();
    if(pid2 < 0)
    {
        perror("ERROR\n");
        exit(1);
    }
    else if(pid2 == 0) // P2 - barrier
    {
        info(BEGIN, 2, 0);
        
        pthread_t tids2[NR_TH_P2];
        THStruct params[NR_TH_P2];
        
        for(int i = 0; i < NR_TH_P2; i++)
        {
            params[i].idThread = i + 1;
            params[i].idProcess = 2;
            pthread_create(&tids2[i], NULL, task2, &params[i]);
        }
        for(int i = 0; i < NR_TH_P2; i++)
            pthread_join(tids2[i], NULL);

        pid_t pid3 = -1;
        pid3 = fork();
        if(pid3 < 0)
        {
            perror("ERROR\n");
            exit(1);
        }
        else if(pid3 == 0) // P3
        {
            info(BEGIN, 3, 0);

            pid_t pid5 = -1;
            pid5 = fork();
            if(pid5 < 0)
            {
                perror("ERROR\n");
                exit(1);
            }
            else if(pid5 == 0) // P5 - different processes synchronisation
            {
                info(BEGIN, 5, 0);

                pid_t pid6 = -1;
                pid6 = fork();
                if(pid6 < 0)
                {
                    perror("ERROR\n");
                    exit(1);
                }
                else if(pid6 == 0) // P6 - same process synchronisation
                {
                    info(BEGIN, 6, 0);
                    pthread_t tids6[NR_TH_P6];
                    THStruct params[NR_TH_P6];
                    // ... 6.4 ... 6.1 ... 6.1 ... 6.4 ...
                    for(int i = 0; i < NR_TH_P6; i++){
                        params[i].idThread = i + 1;
                        params[i].idProcess = 6;
                        pthread_create(&tids6[i], NULL, task65, &params[i]);
                    }
                    for(int i = 0; i < NR_TH_P6; i++)
                        pthread_join(tids6[i], NULL);
                    info(END, 6, 0);

                }
                else if(pid6 > 0)
                {
                    pthread_t tids5[NR_TH_P5];
                    THStruct params[NR_TH_P5];
                    // ... 5.1 ... 6.3 ... 5.3 ...
                    
                    for(int i = 0; i < NR_TH_P5; i++)
                    {
                        params[i].idThread = i + 1;
                        params[i].idProcess = 5;
                        pthread_create(&tids5[i], NULL, task65, &params[i]);
                    }
                    for(int i = 0; i < NR_TH_P5; i++)
                        pthread_join(tids5[i], NULL);
                    wait(NULL); // P5 waits for P6
                    info(END, 5, 0);
                }
                
            }
            else if(pid5 > 0) // still P3
            {
                wait(NULL); // P3 waits for P5
                info(END, 3, 0);
            }
        }
        else if(pid3 > 0) // still P2
        {
            wait(NULL); // P2 waits for P3
            info(END, 2, 0);
        }
    }
    else if(pid2 > 0) // still P1
    {
        wait(NULL); // P1 waits for P2

        pid_t pid4 = -1;
        pid4 = fork();
        if(pid4 < 0)
        {
            perror("ERROR\n");
            exit(1);
        }
        else if(pid4 == 0) // P4
        {
            info(BEGIN, 4, 0);
            info(END, 4, 0);
        }
        else if(pid4 > 0) // still P1
        {
            wait(NULL); // P1 waits for P4

            pid_t pid7 = -1;
            pid7 = fork();
            if(pid7 < 0)
            {
                perror("ERROR\n");
                exit(1);
            }
            else if(pid7 == 0) // P7
            {
                info(BEGIN, 7, 0);
                info(END, 7, 0);
            }
            else if(pid7 > 0) // still P1
            {
                wait(NULL); // P1 waits for P7
                info(END, 1, 0);
            }
        }
    }
    return 0;
}
