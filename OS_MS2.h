#ifndef OS_MS2_H
#define OS_MS2_H

#include "Queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


#define MAX_MEMORY 60
#define MAX_PROCESSES 5
#define MAX_INSTRUCTIONS 15
#define MAX_LINE_LENGTH 256

// Structure definitions
typedef struct {
    char str[MAX_LINE_LENGTH];
} Line;

typedef struct {
    char Name[30];
    char Data[50];
} MemoryWord;

typedef struct {
    char name[20];       // "userInput", "userOutput", "file"
    int is_locked;       // 0=unlocked, 1=locked
    int locked_by_pid;   // PID of the process holding the lock
    Queue blocked_queue; // Processes waiting for this resource
} Mutex;

// Global variables
extern MemoryWord memory[MAX_MEMORY];
extern int memoryPtr;
extern int processCount;
extern int clock_cycles;
extern int processesArrival[MAX_PROCESSES][2];
extern int lastProcessRR[1][2];
extern char* currentAlgo;
extern int* guiReadyQ;
extern int* guiPCB;
extern char* guiCurInst;
extern int startFlag;
extern int stopFlag;
extern int stepFlag;
extern int finishFlag;
extern bool simKillFlag;
extern Queue all_blocked_queue;
extern Queue ready_queue;
extern Queue rrQueue;
extern Queue fcfsQueue;
extern Queue MLF1Queue;
extern Queue MLF2Queue;
extern Queue MLF3Queue;
extern Queue MLF4Queue;
extern int current_running_pid;
extern Mutex mutexes[3];

// Function declarations
void start();
void stop();
void step();
void finish();
void resetSimulation();
MemoryWord* getMemory();
int getProcessCount();
int getClockCycles();
Queue getUserInputQ();
Queue getUserOnputQ();
Queue getFileQ();
int getUserInputStatus();
int getUserOnputStatus();
int getFileStatus();
char* getCurrentAlgo();
Queue getFCFSQ();
Queue getRRQ();
Queue getMLF1Q();
Queue getMLF2Q();
Queue getMLF3Q();
Queue getMLF4Q();
int* getReadyQueue();
Queue getBlockedQ();
void getState(int pid, char* state);
int getPC(int pid);
int getMemEnd(int pid);
int getMemStart(int pid);
int getPri(int pid);
int getRunningPID();
char* getCurInst(int pid);
void removeFromQueue(Queue *q, int pid);
int isAllBlocked(Queue *q);
void print(char* x);
void writeFile(char *fileName, char *data);
char *readFile(char *fileName);
void printFromTo(int x, int y);
void initAllQueues();
int getArrivalTime(int pid);
int getTimeInQueue(int pid);
void changePriTo(int pid, char* pri);
void changeStateTo(int pid, char state[]);
void changePCTo(int pid, int pc);
void storeAFor(int pid, char input[]);
void storeBFor(int pid, char input[]);
void storeCFor(int pid, char input[]);
char* readAOf(int pid);
char* readBOf(int pid);
char* readCOf(int pid);
void assignInput(int pid, char input[], char arg1[]);
int removeHighestPri(Queue *q);
void semWait(int pid, char *resource);
void semSignal(int pid, char *resource);
int comp(const void* a, const void* b);
void sortAndFillReadyQueue();
void readProcessAndStore(char* fileName, int arrivalTime);
void executeInstruction(int pid, char* instruction);
void executeProcess(int pid);
int executeProcessTillQuantum(int pid, int Q);
void FCFS();
void RR(int Q);
void MLFQ();
void chooseSchd(int s, int Q);
void ProgramFlow();

#endif // OS_MS2_H