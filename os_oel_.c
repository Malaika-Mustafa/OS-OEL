#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROCESSES 100
#define MEMORY_SIZE 1024

// Structure to represent a process
typedef struct
{
    int pid;
    int arrivalTime;
    int executionTime;
    int remainingTime;
    int size;
    int startAddress;
    bool isAllocated;
    bool isFinished;
    int completionTime; // Added completionTime
} Process;

// Structure to represent a free memory block (hole)
typedef struct
{
    int startAddress;
    int size;
} Hole;

// Simple queue structure for processes waiting for memory
typedef struct
{
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

void initializeQueue(ProcessQueue *q)
{
    q->front = q->rear = -1;
}

bool isQueueEmpty(ProcessQueue *q)
{
    return q->front == -1;
}

void enqueue(ProcessQueue *q, Process p)
{
    if (q->rear == MAX_PROCESSES - 1)
    {
        printf("Queue Overflow!\n");
        return;
    }
    if (q->front == -1)
        q->front = 0;
    q->rear++;
    q->queue[q->rear] = p;
}

Process dequeue(ProcessQueue *q)
{
    Process p = q->queue[q->front];
    if (q->front == q->rear)
        q->front = q->rear = -1;
    else
        q->front++;
    return p;
}

void displayMemoryState()
{
    printf("\n----- Memory State ------\n");
    printf("Total Memory: %d units\n", MEMORY_SIZE);
    printf("Memory Used : %d units\n", memoryUsed);
    printf("Memory Free : %d units\n", MEMORY_SIZE - memoryUsed);

    printf("\nAllocated Processes:\n");
    printf("PID\t\tSize\t\tStart Address\n");
    for (int i = 0; i < processCount; i++)
    {
        if (processes[i].isAllocated)
        {
            printf("%d\t\t%d\t\t%d\n", processes[i].pid, processes[i].size, processes[i].startAddress);
        }
    }

    if (holeCount != 0)
    {
        printf("\nFree Holes:\n");
        printf("Start\t\tSize\n");
        for (int i = 0; i < holeCount; i++)
        {
            printf("%d\t\t%d\n", holes[i].startAddress, holes[i].size);
        }
    }

    if (!isQueueEmpty(&waitingQueue))
    {
        printf("\nSuspended Processes : ");
        for (int i = waitingQueue.front; i <= waitingQueue.rear; i++)
        {
            printf("%d ", waitingQueue.queue[i].pid);
        }
        printf("\n");
    }
}

int compareHoles(const void *a, const void *b)
{
    Hole *h1 = (Hole *)a;
    Hole *h2 = (Hole *)b;
    return h1->startAddress - h2->startAddress;
}

void mergeHoles()
{
    if (holeCount <= 1)
        return;
    qsort(holes, holeCount, sizeof(Hole), compareHoles);
    for (int i = 0; i < holeCount - 1; i++)
    {
        if (holes[i].startAddress + holes[i].size == holes[i + 1].startAddress)
        {
            holes[i].size += holes[i + 1].size;
            for (int j = i + 1; j < holeCount - 1; j++)
            {
                holes[j] = holes[j + 1];
            }
            holeCount--;
            i--;
        }
    }
}

void allocateProcess(Process *p)
{
    for (int i = 0; i < holeCount; i++)
    {
        if (holes[i].size >= p->size)
        {
            p->startAddress = holes[i].startAddress;
            p->isAllocated = true;
            memoryUsed += p->size;

            if (holes[i].size == p->size)
            {
                for (int j = i; j < holeCount - 1; j++)
                    holes[j] = holes[j + 1];
                holeCount--;
            }
            else
            {
                holes[i].startAddress += p->size;
                holes[i].size -= p->size;
            }
            return;
        }
    }

    if (MEMORY_SIZE - memoryUsed >= p->size)
    {
        p->startAddress = 0;
        for (int i = 0; i < processCount; i++)
        {
            if (processes[i].isAllocated)
            {
                if (p->startAddress < processes[i].startAddress + processes[i].size)
                    p->startAddress = processes[i].startAddress + processes[i].size;
            }
        }
        if (p->startAddress + p->size <= MEMORY_SIZE)
        {
            p->isAllocated = true;
            memoryUsed += p->size;
            return;
        }
    }

    printf("Memory full! Process %d is suspended.\n", p->pid);
}

void freeProcess(Process *p, int currentTime)
{
    Hole h;
    h.startAddress = p->startAddress;
    h.size = p->size;
    holes[holeCount++] = h;
    memoryUsed -= p->size;
    p->isAllocated = false;
    p->isFinished = true;
    p->completionTime = currentTime;
    mergeHoles();
}

void simulate()
{
    int time = 0;
    int finishedCount = 0;

    while (finishedCount < processCount)
    {
        printf("\n--- Time: %d ---\n\n", time);
        for (int i = 0; i < processCount; i++)
        {
            if (processes[i].arrivalTime == time && !processes[i].isAllocated && !processes[i].isFinished)
            {
                printf("Process %d arrived.\n", processes[i].pid);
                allocateProcess(&processes[i]);
                if (!processes[i].isAllocated)
                    enqueue(&waitingQueue, processes[i]);
            }
        }

        for (int i = 0; i < processCount; i++)
        {
            if (processes[i].isAllocated)
            {
                processes[i].remainingTime--;
                if (processes[i].remainingTime < 0)
                {
                    printf("Process %d finished execution.\n", processes[i].pid);
                    freeProcess(&processes[i], time);
                    finishedCount++;
                    if (finishedCount == 0)
                        break;

                    int qSize = (waitingQueue.rear - waitingQueue.front) + 1;
                    for (int j = 0; j < qSize; j++)
                    {
                        if (isQueueEmpty(&waitingQueue))
                            break;
                        Process waitingProcess = dequeue(&waitingQueue);
                        allocateProcess(&waitingProcess);
                        if (waitingProcess.isAllocated)
                        {
                            printf("Process %d moved from queue to memory.\n", waitingProcess.pid);
                            for (int k = 0; k < processCount; k++)
                            {
                                if (processes[k].pid == waitingProcess.pid)
                                {
                                    processes[k] = waitingProcess;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            enqueue(&waitingQueue, waitingProcess);
                        }
                    }
                }
            }
        }

        displayMemoryState();
        time++;
    }
}

void printTurnaroundTimes()
{
    printf("\n\n\t--- Process Turnaround Times ---\n");
    printf("Process\tArrival Time\tService Time\tTurnaround Time\n");

    int totalTurnaroundTime = 0;
    for (int i = 0; i < processCount; i++)
    {
        int turnaroundTime = processes[i].completionTime - processes[i].arrivalTime;
        totalTurnaroundTime += turnaroundTime;
        printf("%d\t\t%d\t\t%d\t\t%d\n",
               processes[i].pid,
               processes[i].arrivalTime,
               processes[i].executionTime,
               turnaroundTime);
    }
    float averageTurnaroundTime = (float)totalTurnaroundTime / processCount;

    printf("\nThe Average Turnaround Time: %.2f units\n", averageTurnaroundTime );

}

int main()
{
    FILE *file;
    char filename[100];

    printf("\n\t\t_____________ Dynamic Partitioning Memory Management Simulation _____________\n");
    printf("Enter the input filename (e.g., processes.txt): ");
    scanf("%s", filename);

    file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error: Could not open file %s\n", filename);
        return 1;
    }

    fscanf(file, "%d", &processCount);

    if (processCount < 3)
    {
        printf("Error: At least 3 processes required.\n");
        fclose(file);
        return 1;
    }

    initializeQueue(&waitingQueue);

    for (int i = 0; i < processCount; i++)
    {
        processes[i].pid = i + 1;
        if (fscanf(file, "%d %d %d", &processes[i].arrivalTime, &processes[i].executionTime, &processes[i].size) != 3)
        {
            printf("Error: Incorrect format in file.\n");
            fclose(file);
            return 1;
        }
        processes[i].remainingTime = processes[i].executionTime;
        processes[i].isAllocated = false;
        processes[i].isFinished = false;
        processes[i].completionTime = 0;
    }

    fclose(file);

    simulate();

    printTurnaroundTimes();

    printf("\t\t\n________________ Simulation Complete __________________\n");

    return 0;
}
