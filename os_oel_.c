#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_PROCESSES 100
#define MEMORY_SIZE 1024

// Structure to represent a process
typedef struct {
    int pid;
    int arrivalTime;
    int executionTime;
    int remainingTime;
    int size;
    int startAddress;
    bool isAllocated;
    bool isFinished;
} Process;

// Structure to represent a free memory block (hole)
typedef struct {
    int startAddress;
    int size;
 
} Hole;

// Simple queue structure for processes waiting for memory
typedef struct {
    int front, rear;
    Process queue[MAX_PROCESSES];
} ProcessQueue;

// Global arrays and variables
Process processes[MAX_PROCESSES];
Hole holes[MAX_PROCESSES];
ProcessQueue waitingQueue;

int processCount = 0;
int holeCount = 0;
int memoryUsed = 0;

// Initialize the waiting queue
void initializeQueue(ProcessQueue *q) {
    q->front = q->rear = -1;
}

// Check if the queue is empty
bool isQueueEmpty(ProcessQueue *q) {
    return q->front == -1;
}

// Add a process to the queue
void enqueue(ProcessQueue *q, Process p) {
    if (q->rear == MAX_PROCESSES - 1) {
        printf("Queue Overflow!\n");
        return;
    }
    if (q->front == -1)
        q->front = 0;
    q->rear++;
    q->queue[q->rear] = p;
}

// Remove and return the first process from the queue
Process dequeue(ProcessQueue *q) {
    Process p = q->queue[q->front];
    if (q->front == q->rear)
        q->front = q->rear = -1;
    else
        q->front++;
    return p;
}

// Display the current memory state
void displayMemoryState() {
    printf("\n----- Memory State ------\n");
    printf("Total Memory: %d units\n", MEMORY_SIZE);
    printf("Memory Used : %d units\n", memoryUsed);
    printf("Memory Free : %d units\n", MEMORY_SIZE - memoryUsed);

    // Show allocated processes
    printf("\nAllocated Processes:\n");
    printf("PID\tSize\n");
    for (int i = 0; i < processCount; i++) {
        if (processes[i].isAllocated) {
            printf("%d\t%d\t%d\n", processes[i].pid,  processes[i].size);
        }
    }

    // Show free memory holes
    if (holeCount !=0 ){
        printf("\nFree Holes:\n");
        printf("Start\t\tSize\n");
    for (int i = 0; i < holeCount; i++) {
        printf("%d\t\t%d\n", holes[i].startAddress, holes[i].size);
    }
    }
    // Show blocked (waiting) processes
    if (!isQueueEmpty(&waitingQueue)) {
        printf("\nSuspended Processes : ");
        for (int i = waitingQueue.front; i <= waitingQueue.rear; i++) {
            printf("%d ", waitingQueue.queue[i].pid);
        }
        printf("\n");
    }
}

// Merge adjacent holes into a single bigger hole
void mergeHoles() {
    for (int i = 0; i < holeCount - 1; i++) {
        for (int j = i + 1; j < holeCount; j++) {
            if (holes[i].startAddress + holes[i].size == holes[j].startAddress) {
                holes[i].size += holes[j].size;
                for (int k = j; k < holeCount - 1; k++)
                    holes[k] = holes[k + 1];
                holeCount--;
                j--;
            }
        }
    }
}

// Try to allocate memory for a process
void allocateProcess(Process *p) {
    // Check if process can be placed in any existing hole
    for (int i = 0; i < holeCount; i++) {
        if (holes[i].size >= p->size) {
            p->startAddress = holes[i].startAddress;
            p->isAllocated = true;
            memoryUsed += p->size;

            // Update or remove the hole
            if (holes[i].size == p->size) {
                for (int j = i; j < holeCount - 1; j++)
                    holes[j] = holes[j + 1];
                holeCount--;
            } else {
                holes[i].startAddress += p->size;
                holes[i].size -= p->size;
            }
            return;
        }
    }

    // If no hole, check if space is available at the end
    if (MEMORY_SIZE - memoryUsed >= p->size) {
        p->startAddress = 0;
        for (int i = 0; i < processCount; i++) {
            if (processes[i].isAllocated) {
                if (p->startAddress < processes[i].startAddress + processes[i].size)
                    p->startAddress = processes[i].startAddress + processes[i].size;
            }
        }
        if (p->startAddress + p->size <= MEMORY_SIZE) {
            p->isAllocated = true;
            memoryUsed += p->size;
            return;
        }
    }

    // If no space available anywhere
    printf("Memory full! Process %d is suspended.\n", p->pid);
}

// Free memory of a completed process
void freeProcess(Process *p) {
    Hole h;
    h.startAddress = p->startAddress;
    h.size = p->size;
    holes[holeCount++] = h;
    memoryUsed -= p->size;
    p->isAllocated = false;
    p->isFinished = true;
    mergeHoles();
}

// Run the simulation
void simulate() {
    int time = 0;
    int finishedCount = 0;

    while (finishedCount < processCount) {
        // Check for newly arriving processes at this time
        printf("\n--- Time: %d ---\n\n", time);
        for (int i = 0; i < processCount; i++) {
            if (processes[i].arrivalTime == time && !processes[i].isAllocated && !processes[i].isFinished) {
                printf("Process %d arrived.\n", processes[i].pid);
                allocateProcess(&processes[i]);
                if (!processes[i].isAllocated)
                    enqueue(&waitingQueue, processes[i]);
            }
        }

        // Execute allocated processes
        for (int i = 0; i < processCount; i++) {
            if (processes[i].isAllocated) {
                processes[i].remainingTime--;
                if (processes[i].remainingTime < 0) {
                    printf("Process %d finished execution.\n", processes[i].pid);
                    freeProcess(&processes[i]);
                    finishedCount++;
                    if(finishedCount==0) break;

                    // Try to allocate waiting processes after freeing memory
                    int qSize = (waitingQueue.rear - waitingQueue.front) + 1;
                    for (int j = 0; j < qSize; j++) {
                        if (isQueueEmpty(&waitingQueue)) break;
                        Process waitingProcess = dequeue(&waitingQueue);
                        allocateProcess(&waitingProcess);
                        if (waitingProcess.isAllocated) {
                            printf("Process %d moved from queue to memory.\n", waitingProcess.pid);
                            // Update the actual process record
                            for (int k = 0; k < processCount; k++) {
                                if (processes[k].pid == waitingProcess.pid) {
                                    processes[k] = waitingProcess;
                                    break;
                                }
                            }
                        } else {
                            enqueue(&waitingQueue, waitingProcess);
                        }
                    }
                }
            }
        }

        // printf("\n--- Time: %d ---\n", time); //move to up //corect logic
        displayMemoryState();

        time++;
    }
}

int main() {
    printf("\n\t\t_____________ Dynamic Partitioning Memory Management Simulation _____________\n");
    printf("Enter the total number of processes (minimum 10): ");
    scanf("%d", &processCount);
    //exception handling 1
    while (true){
    if (processCount < 5) {
        printf("At least 10 process required.\n");
        main();
     }
     else{
        break;
     }
    }

    initializeQueue(&waitingQueue);

    // Input details for each process
    for (int i = 0; i < processCount; i++) {
        processes[i].pid = i + 1;
        printf("\nEnter details for Process %d:\n", i + 1);
        printf("Arrival Time: ");
        scanf("%d", &processes[i].arrivalTime);
        printf("Execution Time: ");
        scanf("%d", &processes[i].executionTime);
        processes[i].remainingTime = processes[i].executionTime;
        printf("Size (in MBs): ");
        scanf("%d", &processes[i].size);
        processes[i].isAllocated = false;
        processes[i].isFinished = false;
    }

    simulate();

    printf("\t\t\n________________ Simulation Complete __________________\n");

}
