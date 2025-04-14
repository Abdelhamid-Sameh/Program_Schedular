#include <stdio.h>
#include <Queue.c>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char str[256];
} Line;


typedef struct {
    char Name[30];
    char Data[50];
} MemoryWord;

typedef struct
{
    char name[20];       // "userInput", "userOutput", "file"
    int is_locked;       // 0=unlocked, 1=locked
    int locked_by_pid;   // PID of the process holding the lock
    Queue blocked_queue; // Processes waiting for this resource
} Mutex;

MemoryWord memory[60];

int memoryPtr = 0;
int processCount = 0;

int clock_cycles = 0;

int processesArrival[5][2];

Queue all_blocked_queue;
Queue ready_queue;

int current_running_pid = -1;


int comp(const void* a, const void* b) {
    return ((int*)a)[1] - ((int*)b)[1];
}

void ProgramFlow(){
    initAllQueues();
    printf("Please choose a scheduling algorithm :\n");
    printf("1.FCFS\n2.RR\n3.MLFQ");
    int s;
    int Q;
    scanf("%d",&s);
    if(s == 2){
        printf("Please enter the quantum for RR :\n");
        scanf("%d",&Q);
    }
    chooseSchd(s,Q);
}

void chooseSchd(int s,int Q){
    
}

void sortAndFillReadyQueue(){
    qsort(processesArrival, 5, sizeof(int), comp);

    for (int i = 0; i < processCount; i++)
    {
        if(!processesArrival[i]){
            break;
        }
        else{
            enqueue(&ready_queue,processesArrival[i][0]);
        }
    }
    
}

void readProcessAndStore(char* fileName,int arrivalTime){

}

void executeProcess(MemoryWord PCB[5]){

}

int executeProcessTillQuantum(MemoryWord PCB[5],int Q){

}

void FCFS(){
    Queue fcfsQueue;
    initQueue(&fcfsQueue);
    
    while (!isEmpty(&ready_queue))
    {
        int pid = peek(&ready_queue);
        int arrTime = getArrivalTime(pid);
        MemoryWord PCB[5] = loadPCB(pid);
        if(arrTime <= clock_cycles){
            enqueue(&fcfsQueue,dequeue(&ready_queue));            
            current_running_pid = pid;
            executeProcess(PCB);
            dequeue(&fcfsQueue);
        }
        else{
            clock_cycles++;
        }
    }
    
}

int getArrivalTime(int pid){
    for (int i = 0; i < 5; i++)
    {
        if(!processesArrival[i]){
            break;
        }
        else if(processesArrival[i][0] == pid){
            return processesArrival[i][1];
        }
    }
    return -1;
    
}

void RR(int Q){
    Queue rrQueue;
    initQueue(&rrQueue);

    while(!isEmpty(&ready_queue) || !isEmpty(&rrQueue)){
        int arrTime;

        if(!isEmpty(&ready_queue)){
            do
            {
                int pid = peek(&ready_queue);
                arrTime = getArrivalTime(pid);
                MemoryWord PCB[5] = loadPCB(pid);
                if(arrTime <= clock_cycles){
                    enqueue(&rrQueue,dequeue(&ready_queue));
                }
            }while(arrTime <= clock_cycles);
        }
        
        if(!isEmpty(&rrQueue)){
            int pid = peek(&rrQueue);
            MemoryWord PCB[5] = loadPCB(pid);
            while(strcmp(PCB[1].Data,"Blocked") == 0){
                enqueue(&rrQueue,dequeue(&rrQueue));
                pid = peek(&rrQueue);
                MemoryWord PCB[5] = loadPCB(pid);
            }

            MemoryWord PCB[5] = loadPCB(pid);
            dequeue(&rrQueue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(PCB,Q);
            if(rem > 0){
                enqueue(&rrQueue,pid);
            }
        }
        else{
            clock_cycles++;
        }
    }
    
}

void MLFQ(){
    Queue MLF1Queue;
    Queue MLF2Queue;
    Queue MLF3Queue;
    Queue MLF4Queue;

    initQueue(&MLF1Queue);
    initQueue(&MLF2Queue);
    initQueue(&MLF3Queue);
    initQueue(&MLF4Queue);

    while(!isEmpty(&ready_queue) || !isEmpty(&MLF1Queue) || !isEmpty(&MLF2Queue) || !isEmpty(&MLF3Queue) || !isEmpty(&MLF4Queue)){
        int arrTime;

        if(!isEmpty(&ready_queue)){
            do
            {
                int pid = peek(&ready_queue);
                arrTime = getArrivalTime(pid);
                MemoryWord PCB[5] = loadPCB(pid);
                if(arrTime <= clock_cycles){
                    enqueue(&MLF1Queue,dequeue(&ready_queue));
                    changePriTo(pid,"1");
                }
            }while(arrTime <= clock_cycles);
        }

        if(!isEmpty(&MLF1Queue)){
            int pid = peek(&MLF1Queue);
            MemoryWord PCB[5] = loadPCB(pid);
            while(strcmp(PCB[1].Data,"Blocked") == 0){
                enqueue(&MLF1Queue,dequeue(&MLF1Queue));
                pid = peek(&MLF1Queue);
                MemoryWord PCB[5] = loadPCB(pid);
            }

            MemoryWord PCB[5] = loadPCB(pid);
            dequeue(&MLF1Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(PCB,1);
            if(rem > 0){
                enqueue(&MLF2Queue,pid);
                changePriTo(pid,"2");
            }
        }

        else if(!isEmpty(&MLF2Queue)){
            int pid = peek(&MLF2Queue);
            MemoryWord PCB[5] = loadPCB(pid);
            while(strcmp(PCB[1].Data,"Blocked") == 0){
                enqueue(&MLF2Queue,dequeue(&MLF2Queue));
                pid = peek(&MLF2Queue);
                MemoryWord PCB[5] = loadPCB(pid);
            }

            MemoryWord PCB[5] = loadPCB(pid);
            dequeue(&MLF2Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(PCB,2);
            if(rem > 0){
                enqueue(&MLF3Queue,pid);
                changePriTo(pid,"3");
            }
        }

        else if(!isEmpty(&MLF3Queue)){
            int pid = peek(&MLF3Queue);
            MemoryWord PCB[5] = loadPCB(pid);
            while(strcmp(PCB[1].Data,"Blocked") == 0){
                enqueue(&MLF3Queue,dequeue(&MLF3Queue));
                pid = peek(&MLF3Queue);
                MemoryWord PCB[5] = loadPCB(pid);
            }

            MemoryWord PCB[5] = loadPCB(pid);
            dequeue(&MLF3Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(PCB,4);
            if(rem > 0){
                enqueue(&MLF4Queue,pid);
                changePriTo(pid,"4");
            }
        }

        else if(!isEmpty(&MLF4Queue)){
            int pid = peek(&MLF4Queue);
            MemoryWord PCB[5] = loadPCB(pid);
            while(strcmp(PCB[1].Data,"Blocked") == 0){
                enqueue(&MLF4Queue,dequeue(&MLF4Queue));
                pid = peek(&MLF4Queue);
                MemoryWord PCB[5] = loadPCB(pid);
            }

            MemoryWord PCB[5] = loadPCB(pid);
            dequeue(&MLF4Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(PCB,8);
            if(rem > 0){
                enqueue(&MLF4Queue,pid);
            }
        }

        else{
            clock_cycles++;
        }
    }

}

MemoryWord* loadPCB(int PID){
    MemoryWord PCB[5];

    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0){
            if(atoi(memory[i].Data) == PID){
                int j = 0;
                for (int k = i; k < i+5; k++)
                {
                    PCB[j] = memory[i];
                    j++;
                }
                break;
            }
        }
    }

    return PCB;
    
}

void removeFromQueue(Queue *q, int pid)
{
    Queue temp;
    initQueue(&temp);

    while (!isEmpty(q))
    {
        int val = dequeue(q);
        if (val != pid)
            enqueue(&temp, val);
    }

    while (!isEmpty(&temp))
    {
        enqueue(q, dequeue(&temp));
    }
}

void print(char* x){
    printf("%s",x);
}

void assignInputInt(int y){
    
}

void assignInputStr(char* y){
    
}

void writeFile(char *fileName, char *data)
{
   
}

char *readFile(char *fileName)
{
    
}

void printFromTo(int x, int y)
{
    
}

Mutex mutexes[3] = {
    {"userInput", 0, -1, {0}},  // For input operations
    {"userOutput", 0, -1, {0}}, // For print operations
    {"file", 0, -1, {0}}        // For file operations
};

void semWait(char *resource)
{
    
}

void semSignal(char *resource)
{
    
}

void initAllQueues()
{
    initQueue(&ready_queue);
    initQueue(&all_blocked_queue);

    // Initialize mutex queues
    for (int i = 0; i < 3; i++)
    {
        initQueue(&mutexes[i].blocked_queue);
    }
}

void changePriTo(int pid, char* pri){
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            strcpy(memory[i+2].Data,pri);
            break;
        }
    }
    
}

void changeStateTo(int pid, char* state){
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            strcpy(memory[i+1].Data,state);
            break;
        }
    }
    
}