#include <stdio.h>
#include "Queue.h"
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

void getState(int pid,char* state){
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            strcpy(state,memory[i+1].Data);
            break;
        }
    }
}

int getPC(int pid){
    int PC;
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            PC = atoi(memory[i+3].Data);
            break;
        }
    }
    return PC;
}

int getMemEnd(int pid){
    int memEnd;
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            memEnd = atoi(memory[i+5].Data);
            break;
        }
    }
    return memEnd;
}

int getPri(int pid){
    int Pri;
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            Pri = atoi(memory[i+2].Data);
            break;
        }
    }
    return Pri;
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

int isAllBlocked(Queue *q){
    int result = 1;

    Queue temp;
    initQueue(&temp);

    while (!isEmpty(q))
    {
        int val = dequeue(q);
        char state[50];
        getState(val,state);
        if(strcmp(state,"Blocked") != 0){
            result = 0;
        }
        enqueue(&temp,val);
    }

    while (!isEmpty(&temp))
    {
        enqueue(q, dequeue(&temp));
    }

    return result;
}

void print(char* x){
    printf("%s\n",x);
}

void writeFile(char *fileName, char *data)
{
    FILE *file = fopen(fileName, "w");
    if (file)
    {
        fprintf(file, "%s", data);
        fclose(file);
    }
}

char *readFile(char *fileName)
{
    // Static buffer to hold content (persists after function returns)
    static char content[256];

    FILE *file = fopen(fileName, "r");
    if (file)
    {
        if (fgets(content, sizeof(content), file))
        {
            // Remove trailing newline if present
            content[strcspn(content, "\n")] = '\0';
        }
        else
        {
            strcpy(content, "FILE_READ_ERROR");
        }
        fclose(file);
    }
    else
    {
        strcpy(content, "FILE_NOT_FOUND");
    }

    return content;
}

void printFromTo(int x, int y)
{
    printf("Printing from %d to %d: ", x, y);
    if (x <= y)
    {
        for (int i = x; i <= y; i++)
            printf("%d ", i);
    }
    else
    {
        for (int i = x; i >= y; i--)
            printf("%d ", i);
    }
    printf("\n");
}

Mutex mutexes[3] = {
    {"userInput", 0, -1, {}},  // For input operations
    {"userOutput", 0, -1, {}}, // For print operations
    {"file", 0, -1, {}}        // For file operations
};

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

void changePriTo(int pid, char* pri){
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            strcpy(memory[i+2].Data,pri);
            break;
        }
    }
    
}

void changeStateTo(int pid, char state[]){
    for (int i = 0; i < memoryPtr; i++)
    {
        if(strcmp(memory[i].Name,"PID") == 0 && atoi(memory[i].Data) == pid){
            strcpy(memory[i+1].Data,state);
            break;
        }
    }
    
}

void changePCTo(int pid,int pc){
    for (int i = 0; i < memoryPtr; i++) {
        if (strcmp(memory[i].Name, "PID") == 0 && atoi(memory[i].Data) == pid) {
            sprintf(memory[i+3].Data, "%d", pc); 
            break;
        }
    }
}

void storeAFor(int pid, char input[]){
    int memEnd = getMemEnd(pid);
    int a = memEnd - 2;
    strcpy(memory[a].Data,input);
}

void storeBFor(int pid, char input[]){
    int memEnd = getMemEnd(pid);
    int b = memEnd - 1;
    strcpy(memory[b].Data,input);
}

void storeCFor(int pid, char input[]){
    int memEnd = getMemEnd(pid);
    int c = memEnd;
    strcpy(memory[c].Data,input);
}

char* readAOf(int pid){
    int memEnd = getMemEnd(pid);
    int a = memEnd - 2;
    return memory[a].Data;
}

char* readBOf(int pid){
    int memEnd = getMemEnd(pid);
    int b = memEnd - 1;
    return memory[b].Data;
}

char* readCOf(int pid){
    int memEnd = getMemEnd(pid);
    int c = memEnd;
    return memory[c].Data;
}

void assignInput(int pid, char input[], char arg1[]){
    if(strcmp(arg1,"a") == 0){
        storeAFor(pid,input);
    }
    else if (strcmp(arg1,"b") == 0){
        storeBFor(pid,input);
    }
    else{
        storeCFor(pid,input);
    }
}

int removeHighestPri(Queue *q)
{
    Queue temp;
    initQueue(&temp);

    int maxPri = 5;
    int pidOfMaxPri = -1;

    while (!isEmpty(q))
    {
        int val = dequeue(q);
        int pri = getPri(val);
        if(pri < maxPri){
            maxPri = pri;
            pidOfMaxPri = val;
        }
        enqueue(&temp, val);
    }

    while (!isEmpty(&temp))
    {
        enqueue(q, dequeue(&temp));
    }

    return pidOfMaxPri;
}

void semWait(int pid,char *resource)
{
    for (int i = 0; i < 3; i++)
    {
        if (strcmp(mutexes[i].name, resource) == 0)
        {
            if (mutexes[i].is_locked)
            {
                // Block the process
                
                changeStateTo(pid,"Blocked");
                enqueue(&mutexes[i].blocked_queue, pid);
                enqueue(&all_blocked_queue, pid);
            }
            else
            {
                // Acquire the resource
                mutexes[i].is_locked = 1;
                mutexes[i].locked_by_pid = pid;
            }
            return;
        }
    }
    printf("Error: Unknown resource '%s'\n", resource);
}

void semSignal(int pid,char *resource)
{
    for (int i = 0; i < 3; i++)
    {
        if (strcmp(mutexes[i].name, resource) == 0)
        {
            if (!isEmpty(&mutexes[i].blocked_queue))
            {
                // Unblock the highest-priority process
                int unblocked_pid = removeHighestPri(&mutexes[i].blocked_queue);
                removeFromQueue(&mutexes[i].blocked_queue,unblocked_pid);
                removeFromQueue(&all_blocked_queue, unblocked_pid);

                changeStateTo(unblocked_pid,"Ready");
            }
            else
            {
                // No waiters - release resource
                mutexes[i].is_locked = 0;
                mutexes[i].locked_by_pid = -1;
            }
            return;
        }
    }
}

int comp(const void* a, const void* b) {
    return ((int*)a)[1] - ((int*)b)[1];
}

void sortAndFillReadyQueue(){
    qsort(processesArrival, processCount, sizeof(int[2]), comp);

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
    if(memoryPtr > 45){
        printf("Memory is full !\n");
        return;
    }

    FILE *file = fopen(fileName, "r");
    char line[256];
    Line instructions[15];
    int instCount = 0;

    if (file != NULL) {
        while (fgets(line, sizeof(line), file) && instCount < 15) {
            line[strcspn(line, "\n")] = '\0';
            strcpy(instructions[instCount].str, line);
            instCount++;
        }
        fclose(file);
    } else {
        printf("Error: could not open file !");
    }

    int LB = memoryPtr;
    strcpy(memory[memoryPtr].Name, "PID");
    sprintf(memory[memoryPtr].Data, "%d", processCount + 1);
    processesArrival[processCount][0] = processCount + 1;
    processesArrival[processCount][1] = arrivalTime;
    processCount++;
    memoryPtr++;

    strcpy(memory[memoryPtr].Name, "State");
    strcpy(memory[memoryPtr].Data, "Ready");
    memoryPtr++;

    strcpy(memory[memoryPtr].Name, "Pri");
    strcpy(memory[memoryPtr].Data, "0");
    memoryPtr++;

    strcpy(memory[memoryPtr].Name, "PC");
    int startInst = memoryPtr + 3;
    sprintf(memory[memoryPtr].Data, "%d", startInst);
    memoryPtr++;

    strcpy(memory[memoryPtr].Name, "Memory Start");
    sprintf(memory[memoryPtr].Data, "%d", LB);
    memoryPtr++;

    strcpy(memory[memoryPtr].Name, "Memory End");
    int UB = memoryPtr + instCount + 3;
    sprintf(memory[memoryPtr].Data, "%d", UB);
    memoryPtr++;

    for (int i = 0; i < instCount; i++) {
        strcpy(memory[memoryPtr].Name, "Instruction");
        strcpy(memory[memoryPtr].Data, instructions[i].str);
        memoryPtr++;
    }

    strcpy(memory[memoryPtr].Name, "Var a");
    memoryPtr++;
    strcpy(memory[memoryPtr].Name, "Var b");
    memoryPtr++;
    strcpy(memory[memoryPtr].Name, "Var c");
    memoryPtr++;
}

void executeInstruction(int pid, char* instruction){
    char command[20];
    char arg1[50];
    char arg2[50];
    char optionalarg3[50];

    
    int numArgs = sscanf(instruction, "%s %s %s %s", command, arg1, arg2, optionalarg3);
    
    if (strcmp(command, "print") == 0) {
        if(strcmp(arg1, "a") == 0){
            print(readAOf(pid));
        }
        else if(strcmp(arg1, "b") == 0){
            print(readBOf(pid));
        }
        else{
            print(readCOf(pid));
        }
    } 
    else if (strcmp(command, "assign") == 0) {
        if (strcmp(arg2, "input") == 0) {
            printf("Please enter a value: ");
            char input[50];
            scanf("%s", input);
            assignInput(pid,input,arg1);
        } else if(strcmp(arg2, "readFile") == 0){
            char* input;
            char* fileName;
            if(strcmp(optionalarg3, "a") == 0){
                fileName = readAOf(pid);
            }
            else if(strcmp(optionalarg3, "b") == 0){
                fileName = readBOf(pid);
            }
            else{
                fileName = readCOf(pid);
            }
            input = readFile(fileName);
            assignInput(pid,input,arg1);
        }
    } 
    else if (strcmp(command, "writeFile") == 0) {
        char* input;
        char* fileName;
        if(strcmp(arg1, "a") == 0){
            fileName = readAOf(pid);
        }
        else if(strcmp(arg1, "b") == 0){
            fileName = readBOf(pid);
        }
        else{
            fileName = readCOf(pid);
        }

        if(strcmp(arg2, "a") == 0){
            input = readAOf(pid);
        }
        else if(strcmp(arg2, "b") == 0){
            input = readBOf(pid);
        }
        else{
            input = readCOf(pid);
        }
        writeFile(fileName, input);
    } 
    else if (strcmp(command, "printFromTo") == 0) {
        int a;
        int b;

        if(strcmp(arg1, "a") == 0){
            a = atoi(readAOf(pid));
        }
        else if(strcmp(arg1, "b") == 0){
            a = atoi(readBOf(pid));
        }
        else{
            a = atoi(readCOf(pid));
        }

        if(strcmp(arg2, "a") == 0){
            b = atoi(readAOf(pid));
        }
        else if(strcmp(arg2, "b") == 0){
            b = atoi(readBOf(pid));
        }
        else{
            b = atoi(readCOf(pid));
        }
        printFromTo(a, b);
    } 
    else if (strcmp(command, "semWait") == 0) {
        semWait(pid,arg1);
    } 
    else if (strcmp(command, "semSignal") == 0) {
        semSignal(pid,arg1);
    }
    
}


void executeProcess(int pid){
    int pc = getPC(pid);
    int memoryEnd = getMemEnd(pid);

    changeStateTo(pid, "Running");

    while (pc < (memoryEnd - 2)) {
        char* instruction = memory[pc].Data;
        pc++;
        executeInstruction(pid, instruction);
        
        changePCTo(pid,pc);
        
        clock_cycles++;
    }
    
    changeStateTo(pid, "Terminated");
}

int executeProcessTillQuantum(int pid,int Q){
    int pc = getPC(pid);
    int memoryEnd = getMemEnd(pid);
    int clockCount = 0;
    changeStateTo(pid, "Running");

    while (pc < (memoryEnd - 2) && clockCount < Q) {
        char state[50];
        getState(pid,state);

        if(strcmp(state,"Blocked") == 0){
            break;
        }

        char* instruction = memory[pc].Data;
        pc++;
        executeInstruction(pid, instruction);
        clockCount++;
        
        changePCTo(pid,pc);
        
        clock_cycles++;
    }
    if(pc == (memoryEnd - 2)){
        changeStateTo(pid, "Terminated");
        return 0;
    }
    return (memoryEnd - 2) - pc;
}

void FCFS(){
    Queue fcfsQueue;
    initQueue(&fcfsQueue);

    while (!isEmpty(&ready_queue))
    {
        int pid = peek(&ready_queue);
        int arrTime = getArrivalTime(pid);
        if(arrTime <= clock_cycles){
            enqueue(&fcfsQueue,dequeue(&ready_queue));            
            current_running_pid = pid;
            executeProcess(pid);
            dequeue(&fcfsQueue);
        }
        else{
            clock_cycles++;
        }
    }
    
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
                if(arrTime <= clock_cycles){
                    enqueue(&rrQueue,dequeue(&ready_queue));
                }
            }while(arrTime <= clock_cycles && !isEmpty(&ready_queue));
        }
        
        if(!isEmpty(&rrQueue)){
            int pid = peek(&rrQueue);
            char state[50];
            getState(pid,state);
            while(strcmp(state,"Blocked") == 0){
                enqueue(&rrQueue,dequeue(&rrQueue));
                pid = peek(&rrQueue);
                getState(pid,state);
            }
            dequeue(&rrQueue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(pid,Q);
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
                if(arrTime <= clock_cycles){
                    enqueue(&MLF1Queue,dequeue(&ready_queue));
                    changePriTo(pid,"1");
                }
            }while(arrTime <= clock_cycles && !isEmpty(&ready_queue));
        }

        if(!isEmpty(&MLF1Queue) && !isAllBlocked(&MLF1Queue)){
            int pid = peek(&MLF1Queue);
            char state[50];
            getState(pid,state);
            while(strcmp(state,"Blocked") == 0){
                enqueue(&MLF1Queue,dequeue(&MLF1Queue));
                pid = peek(&MLF1Queue);
                getState(pid,state);
            }

            dequeue(&MLF1Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(pid,1);
            if(rem > 0){
                enqueue(&MLF2Queue,pid);
                changePriTo(pid,"2");
            }
        }

        else if(!isEmpty(&MLF2Queue) && !isAllBlocked(&MLF2Queue)){
            int pid = peek(&MLF2Queue);
            char state[50];
            getState(pid,state);
            while(strcmp(state,"Blocked") == 0){
                enqueue(&MLF2Queue,dequeue(&MLF2Queue));
                pid = peek(&MLF2Queue);
                getState(pid,state);
            }

            dequeue(&MLF2Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(pid,2);
            if(rem > 0){
                enqueue(&MLF3Queue,pid);
                changePriTo(pid,"3");
            }
        }

        else if(!isEmpty(&MLF3Queue) && !isAllBlocked(&MLF3Queue)){
            int pid = peek(&MLF3Queue);
            char state[50];
            getState(pid,state);
            while(strcmp(state,"Blocked") == 0){
                enqueue(&MLF3Queue,dequeue(&MLF3Queue));
                pid = peek(&MLF3Queue);
                getState(pid,state);
            }

            dequeue(&MLF3Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(pid,4);
            if(rem > 0){
                enqueue(&MLF4Queue,pid);
                changePriTo(pid,"4");
            }
        }

        else if(!isEmpty(&MLF4Queue) && !isAllBlocked(&MLF4Queue)){
            int pid = peek(&MLF4Queue);
            char state[50];
            getState(pid,state);
            while(strcmp(state,"Blocked") == 0){
                enqueue(&MLF4Queue,dequeue(&MLF4Queue));
                pid = peek(&MLF4Queue);
                getState(pid,state);
            }

            dequeue(&MLF4Queue);
            current_running_pid = pid;
            int rem = executeProcessTillQuantum(pid,8);
            if(rem > 0){
                enqueue(&MLF4Queue,pid);
            }
        }

        else{
            clock_cycles++;
        }
    }

}

void chooseSchd(int s,int Q){
    sortAndFillReadyQueue();
    switch (s)
    {
    case 1:FCFS();break;
    case 2:RR(Q);break;
    case 3:MLFQ();break;
    default:printf("Invalid Choice !");break;
    }
}

void ProgramFlow(){
    initAllQueues();
    printf("Please choose a scheduling algorithm :\n");
    printf("1.FCFS\n2.RR\n3.MLFQ\n");
    int s;
    int Q;
    scanf("%d",&s);
    if(s == 2){
        printf("Please enter the quantum for RR :\n");
        scanf("%d",&Q);
    }
    chooseSchd(s,Q);
}

int main(){
    readProcessAndStore("Program_1.txt",0);
    readProcessAndStore("Program_2.txt",0);
    readProcessAndStore("Program_3.txt",0);

    for (int i = 0; i < memoryPtr; i++)
    {
        printf("%s : %s\n",memory[i].Name,memory[i].Data);
    }
    
    ProgramFlow();

}
