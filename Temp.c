#include <stdio.h>
#include <Queue.c>

typedef struct {
    char str[256];
} Line;


typedef struct {
    char Name[30];
    char Data[50];
} MemoryWord;

MemoryWord memory[60];

int memoryPtr = 0;
int processCount = 0;

int clock_cycles = 0;

int processesArrival[5][2];

int comp(const void* a, const void* b) {
    return ((int*)a)[1] - ((int*)b)[1];
}

void ProgramFlow(){
   
}

void chooseSchd(int s,int Q){
    
}

void readProcessAndStore(char* fileName,int arrivalTime){
    
}

void executeProcess(MemoryWord PCB[5]){

}

int executeProcessTillQuantum(MemoryWord PCB[5],int Q){

}

void FCFS(){
    qsort(processesArrival, 5, sizeof(int), comp);

    Queue fcfsQueue;
    initQueue(&fcfsQueue);
    for (int i = 0; i < 5; i++)
    {
        if(!processesArrival[i]){
            break;
        }
        else{
            enqueue(&fcfsQueue,processesArrival[i][0]);
        }
    }

    while(!isEmpty(&fcfsQueue)){
        int pid = peek(&fcfsQueue);
        int arrTime = getArrivalTime(pid);
        if(arrTime <= clock_cycles){
            dequeue(&fcfsQueue);
            MemoryWord PCB[5] = loadPCB(pid);
            executeProcess(PCB);
        }
        clock_cycles++;
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
    qsort(processesArrival, 5, sizeof(int), comp);

    Queue rrQueue;
    initQueue(&rrQueue);
    for (int i = 0; i < 5; i++)
    {
        if(!processesArrival[i]){
            break;
        }
        else{
            enqueue(&rrQueue,processesArrival[i][0]);
        }
    }

    while(!isEmpty(&rrQueue)){
        int pid = peek(&rrQueue);
        int arrTime = getArrivalTime(pid);
        if(arrTime == clock_cycles){
            dequeue(&rrQueue);
            MemoryWord PCB[5] = loadPCB(pid);
            int rem = executeProcessTillQuantum(PCB,Q);
            if(rem > 0){
                enqueue(&rrQueue,pid);
            }
        }
        clock_cycles++;
    }
    
}

void MLFQ(){
    qsort(processesArrival, 5, sizeof(int), comp);

    Queue MLF1Queue;
    Queue MLF2Queue;
    Queue MLF3Queue;
    Queue MLF4Queue;

    initQueue(&MLF1Queue);
    initQueue(&MLF2Queue);
    initQueue(&MLF3Queue);
    initQueue(&MLF4Queue);

    for (int i = 0; i < 5; i++)
    {
        if(!processesArrival[i]){
            break;
        }
        else{
            enqueue(&MLF1Queue,processesArrival[i][0]);
        }
    }
    
    while(!isEmpty(&MLF1Queue) && !isEmpty(&MLF2Queue) && !isEmpty(&MLF3Queue) && !isEmpty(&MLF4Queue)){
        if(!isEmpty(&MLF1Queue)){

        }
        else if(!isEmpty(&MLF2Queue)){

        }
        else if(!isEmpty(&MLF3Queue)){
            
        }
        else if(!isEmpty(&MLF4Queue)){
            
        }
    }
}

MemoryWord* loadPCB(int PID){
    MemoryWord PCB[5];

    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID")){
            if(atoi(memory[i].Data == PID)){
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

void print(char* x){
    
}

void assignInputInt(int y){
    
}

void assignInputStr(char* y){
    
}

void writeFile(char* fileName,char* data){
    
}

char* readFile(char* fileName){
    
}

void printFromTo(int x,int y){
    
}

void semWait(char* resName){
    
}

void semSignal(char* resName){
    
}