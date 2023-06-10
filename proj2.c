// File: proj2.c
// Subject: IOS
// Project: #2
// Author: Andrii Klymenko
// Login: xklyme00

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

// Semaphore pointers
sem_t *s_writeFile = NULL;

sem_t *s_mailQueue = NULL;
sem_t *s_parcelQueue = NULL;
sem_t *s_moneyQueue = NULL;

sem_t *s_mailMutex = NULL;
sem_t *s_parcelMutex = NULL;
sem_t *s_moneyMutex = NULL;
sem_t *s_closingMutex = NULL;

// Shared memory pointers
int *mailQueueCounter = NULL;
int *parcelQueueCounter = NULL;
int *moneyQueueCounter = NULL;

int *sequenceCounter = NULL;

bool *isOpened = NULL;

// Output file pointer
FILE *fp;

// Semaphore and shared memory initialization
int initialize()
{
    if ((s_writeFile = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;

    if ((s_mailQueue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;
    if ((s_parcelQueue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;
    if ((s_moneyQueue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;

    if ((s_mailMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;
    if ((s_parcelMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;
    if ((s_moneyMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;
    if ((s_closingMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;

    if ((mailQueueCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;
    if ((parcelQueueCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;
    if ((moneyQueueCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;

    if ((sequenceCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;

    if ((isOpened = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return -1;

    if ((sem_init(s_writeFile, 1, 1) == -1)) return -1;

    if ((sem_init(s_mailQueue, 1, 0) == -1)) return -1;
    if ((sem_init(s_parcelQueue, 1, 0) == -1)) return -1;
    if ((sem_init(s_moneyQueue, 1, 0) == -1)) return -1;

    if ((sem_init(s_mailMutex, 1, 1) == -1)) return -1;
    if ((sem_init(s_parcelMutex, 1, 1) == -1)) return -1;
    if ((sem_init(s_moneyMutex, 1, 1) == -1)) return -1;
    if ((sem_init(s_closingMutex, 1, 1) == -1)) return -1;

    return 0;
}

// Semaphore and shared memory cleanup
void destroy()
{
    sem_destroy(s_writeFile);

    sem_destroy(s_mailQueue);
    sem_destroy(s_parcelQueue);
    sem_destroy(s_moneyQueue);

    sem_destroy(s_mailMutex);
    sem_destroy(s_parcelMutex);
    sem_destroy(s_moneyMutex);
    sem_destroy(s_closingMutex);
    
    munmap(s_writeFile, sizeof(sem_t));

    munmap(s_mailQueue, sizeof(sem_t));
    munmap(s_parcelQueue, sizeof(sem_t));
    munmap(s_moneyQueue, sizeof(sem_t));

    munmap(s_mailMutex, sizeof(sem_t));
    munmap(s_parcelMutex, sizeof(sem_t));
    munmap(s_moneyMutex, sizeof(sem_t));
    munmap(s_closingMutex, sizeof(sem_t));

    munmap(mailQueueCounter, sizeof(int));
    munmap(parcelQueueCounter, sizeof(int));
    munmap(moneyQueueCounter, sizeof(int));

    munmap(sequenceCounter, sizeof(int));

    munmap(isOpened, sizeof(bool));
}

// Print function for easier printing
// Print given string to output file with sequence
// Semaphore is used to prevent problems with print
void myPrint(const char *format, ...)
{
    sem_wait(s_writeFile);
    va_list args;
    va_start(args, format);
    fprintf(fp, "%d: ", *sequenceCounter);
    fflush(fp);
    vfprintf(fp, format, args);
    fflush(fp);
    (*sequenceCounter)++;
    va_end(args);
    sem_post(s_writeFile);
}

// Function that is used by customer process
// Customer firstly increments number of customers in the queue where he is going to wait
// and then waits in queue until he is called by an official
void waitInQueue(sem_t *s_queue, sem_t *s_queueMutex, int *queueCounter)
{
    sem_wait(s_queueMutex);
    (*queueCounter)++;
    sem_post(s_queueMutex);
    sem_wait(s_queue);
}

// Customer process
void customerProcess(int idZ, int TZ)
{
    // Set the seed of rand to get truly random numbers
    srand(time(NULL) * getpid());

    // Process started
    myPrint("Z %d: started\n", idZ);

    // Sleep <0, TZ>
    usleep((rand() % (TZ + 1)) * 1000);

    // Closing semaphore in order to avoid printing "entering office ..." after "closing"
    sem_wait(s_closingMutex);
    if(*isOpened)
    {
        // If office is opened, customer enters the office and waits in queue untill he is called by an official

        // Generating a random number from 1 to 3. This number represents the queue number.
        int queueNumber = rand() % 3 + 1;
        myPrint("Z %d: entering office for a service %d\n", idZ, queueNumber);

        // Unlocking closing semaphore, because "entering office ..." was printed
        sem_post(s_closingMutex);

        // Based on queue number, the client enters the right queue and waits
        if(queueNumber == 1)
            waitInQueue(s_mailQueue, s_mailMutex, mailQueueCounter);
        else if(queueNumber == 2)
            waitInQueue(s_parcelQueue, s_parcelMutex, parcelQueueCounter);
        else if(queueNumber == 3)
            waitInQueue(s_moneyQueue, s_moneyMutex, moneyQueueCounter);

        // When official unlocks the semaphore in which customer waits, customer process continues
        myPrint("Z %d: called by office worker\n", idZ);

        // Sleep <0, 10>
        usleep((rand() % 11) * 1000);
    }
    else  // Closing semaphore need to be unlocked
        sem_post(s_closingMutex);

    myPrint("Z %d: going home\n", idZ);

    // Process ended, exit successfully
    exit(0);
}

// Function that is used by official process
// It firstly decrements number of customers in the queue and then unlocks the semaphore of the queue 
// in order to let customer process continue
void serveQueue(sem_t *s_queue, sem_t *s_queueMutex, int *queueCounter)
{
    sem_wait(s_queueMutex);
    (*queueCounter)--;
    sem_post(s_queueMutex);
    sem_post(s_queue);
}

// Function that is used by official process
// Official randomly chooses one of the queues (1, 2 or 3) to be served. If there is no customer in the queue, function returns 0.
int getQueueNumber(int mailQueueCounter, int parcelQueueCounter, int moneyQueueCounter)
{
    if (mailQueueCounter > 0 && parcelQueueCounter > 0 && moneyQueueCounter > 0) {
        return rand() % 3 + 1;
    } else if (mailQueueCounter > 0 && parcelQueueCounter > 0) {
        return rand() % 2 + 1;
    } else if (mailQueueCounter > 0 && moneyQueueCounter > 0) {
        return (rand() % 2) * 2 + 1;
    } else if (mailQueueCounter > 0) {
        return 1;
    } else if (parcelQueueCounter > 0 && moneyQueueCounter > 0) {
        return rand() % 2 + 2;
    } else if (parcelQueueCounter > 0) {
        return 2;
    } else if (moneyQueueCounter > 0) {
        return 3;
    } else {
        return 0;
    }
}

// Official process
void officialProcess(int idU, int TU)
{
    // Set the seed of rand to get truly random numbers
    srand(time(NULL) * getpid());

    // Process started
    myPrint("U %d: started\n", idU);

    // Loop, which is executed if the post is open or at least one of the client queues is not empty
    while(1)
    {
        // Closing semaphore in order to avoid printing "taking break" after "closing"
        sem_wait(s_closingMutex);

        // Check if post is opened or if at least one of the queues is not empty
        if(*isOpened == true || *mailQueueCounter > 0 || *parcelQueueCounter > 0 || *moneyQueueCounter > 0)
        {
            // Get number of queue which will be served by official
            int queueNumber = getQueueNumber(*mailQueueCounter, *parcelQueueCounter, *moneyQueueCounter);

            // Service customer from the queue based on the queue number selected by the official
            if(queueNumber == 1)
                serveQueue(s_mailQueue, s_mailMutex, mailQueueCounter);
            else if(queueNumber == 2)
                serveQueue(s_parcelQueue, s_parcelMutex, parcelQueueCounter);
            else if(queueNumber == 3)
                serveQueue(s_moneyQueue, s_moneyMutex, moneyQueueCounter);
            else
            {
                // If all the queues are empty, official takes a break
                myPrint("U %d: taking break\n", idU);

                // Unlock closing semaphore, because "taking break" was printed
                sem_post(s_closingMutex);

                // Sleep <0, TU>
                usleep((rand() % (TU + 1)) * 1000);
                myPrint("U %d: break finished\n", idU);

                // Go to next iteration of the loop
                continue;
            }

            myPrint("U %d: serving a service of type %d\n", idU, queueNumber);

            // If official did not take a break, closing semaphore need to be unlocked
            sem_post(s_closingMutex);

            // Sleep <0, 10>
            usleep((rand() % 11) * 1000);
            myPrint("U %d: service finished\n", idU);
        }
        else
        {
            // Closing semaphore need to be unlocked
            sem_post(s_closingMutex);
            break;
        }
    }
    myPrint("U %d: going home\n", idU);

    // Process ended, exit successfully
    exit(0);
}

// Function that determines whether the string is an integer. If it is, function returns true, otherwise it returns false.
bool isInteger(char *str)
{
    bool isInteger = true;

    if(strcmp(str, "") == 0)
        isInteger = false;
    else
    {
        for (size_t i = 0; i < strlen(str); i++)
        {
            if(!isdigit(str[i]))
            {
                isInteger = false;
                break;
            }
        }
    }

    return isInteger;
}

// Function that prints printUsage of the program
void printUsage()
{
    printf("printUsage: ./proj2 NZ NU TZ TU F\nNZ - Number of customers\nNU - Number of officials, and NU > 0\nTZ - Max time in milliseconds, that a customer waits before entering the mail after being created, and TZ is in <0, 10000> interval\nTU - Max length milliseconds of the official break, and TU is in <0, 100> interval\nF - Max time in milliseconds after which mail is closed for new arrivals, and F is in <0, 10000> interval\n");
}

int main (int argc, char *argv[])
{
    // Destroy shared memory preventively
    destroy();

    // Checking if the program was run with the required number of arguments (there must be 5 not including the program itself)
    if (argc != 6)
    {
        fprintf(stderr, "Error: the program must be run strictly with 5 arguments (not including the program itself)\n");
        printf("printUsage: ./proj2 NZ NU TZ TU F\n");
        return 1;
    }

    int NZ; // Number of customers
    int NU; // Number of officials
    int TZ; // Max time in milliseconds, that a customer waits before entering the mail after being created
    int TU; // Max length milliseconds of the official break
    int F; // Max time in milliseconds after which mail is closed for new arrivals

    // Loop that checks if arguments are valid
    for(int i = 1; i < argc; i++)
    {
        if(isInteger(argv[i]) == false)
        {
            fprintf(stderr, "Error: all arguments must be positive integers (i.e. must be greater than or equal to 0)\n");
            printUsage();
            return 1;
        }
        else
        {
            switch (i)
            {
            case 1:
                NZ = atoi(argv[i]);
                break;

            case 2:
                NU = atoi(argv[i]);
                if(NU == 0)
                {
                    fprintf(stderr, "Error: NU must be greater than 1\n");
                    printUsage();
                    return 1;
                }
                break;

            case 3:
                TZ = atoi(argv[i]);
                if(TZ > 10000)
                {
                    fprintf(stderr, "Error: TZ must be in the interval <0, 10000>\n");
                    printUsage();
                    return 1;
                }
                break;

            case 4:
                TU = atoi(argv[i]);
                if(TU > 100)
                {
                    fprintf(stderr, "Error: TU must be in the interval <0, 100>\n");
                    printUsage();
                    return 1;
                }
                break;

            case 5:
                F = atoi(argv[i]);
                if(F > 10000)
                {
                    fprintf(stderr, "Error: F must be in the interval <0, 10000>\n");
                    printUsage();
                    return 1;
                }
                break;
            default:
                break;
            }
        }
    }

    // Create file for output and open it for reading
    fp = fopen("proj2.out", "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: could not open/create the file proj2.out\n");
        destroy();
        return 1;
    }

    // Initialize shared memory
    if (initialize() != 0)
    {
        fprintf(stderr, "Error: could not initialize shared memory\n");
        destroy();
        return 1;
    }
    
    // Avoid problems with printing, turning off buffering
    setbuf(fp, NULL);

    // Initialize shared variables
    *mailQueueCounter = 0;
    *parcelQueueCounter = 0;
    *moneyQueueCounter = 0;

    *isOpened = true;
    *sequenceCounter = 1;

    // Loop that creates all customer processes
    for (int i = 1; i <= NZ; i++)
    {
        pid_t customerPid = fork();

        if (customerPid == 0)
        {
            customerProcess(i, TZ);
            return 0;
        }
    }

    // Loop that creates all official processes
    for (int i = 1; i <= NU; i++)
    {
        pid_t officialPid = fork();

        if (officialPid == 0)
        {
            officialProcess(i, TU);
            return 0;
        }
    }

    // Sleep <F/2, F>
    usleep((rand() % ((F / 2) + 1) + (F / 2)) * 1000);

    // Lock closing mutex before changing value of isOpened variable to false and printing closing message
    sem_wait(s_closingMutex);
    *isOpened = false;
    myPrint("closing\n");
    // After printing closing message, unlock closing mutex
    sem_post(s_closingMutex);

    // Wait for all child processes to finish
    while(wait(NULL) > 0);

    fclose(fp);

    // Deallocate resources
    destroy();

    return 0;
}
