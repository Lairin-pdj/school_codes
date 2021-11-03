//
// operating system
// CPU Schedule Simulator
// make by Park Dongjun
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>

#define SEED 10

// process states
#define S_IDLE 0			
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent {
	int procid;
	int doneTime;
	int len;
	struct ioDoneEvent *prev;
	struct ioDoneEvent *next;
} ioDoneEventQueue, *ioDoneEvent;

struct process {
	int id;
	int len;		// for queue
	int targetServiceTime;
	int serviceTime;
	int startTime;
	int endTime;
	char state;
	int priority;
	int saveReg0, saveReg1;
	struct process *prev;
	struct process *next;
} *procTable;

struct process idleProc;
struct process readyQueue;
struct process blockedQueue;
struct process *runningProc;

int cpuReg0, cpuReg1;
int currentTime = 0;
int *procIntArrTime, *procServTime, *ioReqIntArrTime, *ioServTime;

void compute() {
	// DO NOT CHANGE THIS FUNCTION
	cpuReg0 = cpuReg0 + runningProc->id;
	cpuReg1 = cpuReg1 + runningProc->id;
	//printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
}

void initProcTable() {
	int i;
	for(i=0; i < NPROC; i++) {
		procTable[i].id = i;
		procTable[i].len = 0;
		procTable[i].targetServiceTime = procServTime[i];
		procTable[i].serviceTime = 0;
		procTable[i].startTime = 0;
		procTable[i].endTime = 0;
		procTable[i].state = S_IDLE;
		procTable[i].priority = 0;
		procTable[i].saveReg0 = 0;
		procTable[i].saveReg1 = 0;
		procTable[i].prev = NULL;
		procTable[i].next = NULL;
	}
}

void procExecSim(struct process *(*scheduler)()) {
	int pid, qTime = 0, cpuUseTime = 0, nproc = 0, termProc = 0, nioreq = 0;
	char schedule = 0, nextState = S_IDLE;
	int nextForkTime, nextIOReqTime;
	int scheCheck = 0;	
	int putReadyCheck = 0;

	nextForkTime = procIntArrTime[nproc];
	nextIOReqTime = ioReqIntArrTime[nioreq];
	
	runningProc = &idleProc;
	pid = runningProc->id = -1;

	while(1) {
		currentTime++;
		qTime++;
		runningProc->serviceTime++;
		if (runningProc != &idleProc ) cpuUseTime++;
		
		// MUST CALL compute() Inside While loop
		compute(); 
		
		if (runningProc->serviceTime == runningProc->targetServiceTime && runningProc != &idleProc) { /* CASE 4 : the process job done and terminates */
			runningProc->state = S_TERMINATE;
			runningProc->endTime = currentTime;
			runningProc->prev = NULL;
			runningProc->next = NULL;
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
			termProc++;
			if (termProc == NPROC){ /* end */
				break;
			}
			scheCheck = 1;
		}
		if (cpuUseTime == nextIOReqTime && nioreq < NIOREQ) { /* CASE 5: reqest IO operations (only when the process does not terminate) */
			if (runningProc != &idleProc){
				struct ioDoneEvent *temp = &ioDoneEventQueue;     
				while (!(temp->next == NULL || temp->next == &ioDoneEventQueue)){
					temp = temp->next;
				}	
				temp->next = &ioDoneEvent[nioreq];
				ioDoneEvent[nioreq].prev = temp;
				ioDoneEvent[nioreq].next = NULL;
				ioDoneEventQueue.len++;	
				
				ioDoneEvent[nioreq].doneTime = currentTime + ioServTime[nioreq];
				ioDoneEvent[nioreq].procid = runningProc->id;
	
				runningProc->state = S_BLOCKED;	
				runningProc->saveReg0 = cpuReg0;
				runningProc->saveReg1 = cpuReg1;		
				if (qTime != QUANTUM){
					runningProc->priority++;
				}
			}

			nioreq++;
			if (nioreq < NIOREQ){
				nextIOReqTime += ioReqIntArrTime[nioreq];
			}
			scheCheck = 1;
		}
		if (currentTime == nextForkTime && nproc < NPROC) { /* CASE 2 : a new process created */
			struct process *temp = &readyQueue;      //put process in readyqueue
			while (!(temp->next == NULL || temp->next == &readyQueue)){
				temp = temp->next;
			}
			temp->next = &procTable[nproc];
			procTable[nproc].prev = temp;
			procTable[nproc].next = NULL;
			readyQueue.len++;

			procTable[nproc].state = S_READY;   // set process
			procTable[nproc].startTime = currentTime;

			nproc++;
			if (nproc < NPROC){
				nextForkTime += procIntArrTime[nproc];
			}

			if (runningProc != &idleProc && runningProc->state == S_RUNNING){
				putReadyCheck = 1;				
			}
			scheCheck = 1;
		}
		if (ioDoneEventQueue.len > 0){ /*CASE 3 : IO Done Event */
			struct ioDoneEvent *iotemp = ioDoneEventQueue.next;
			struct ioDoneEvent *iotemp2;
			int i;
			for (i = 0; i < ioDoneEventQueue.len; i++){
				if (iotemp->doneTime == currentTime){
					if (procTable[iotemp->procid].targetServiceTime != procTable[iotemp->procid].serviceTime){
						struct process *temp = &readyQueue;     
						while (!(temp->next == NULL || temp->next == &readyQueue)){
							temp = temp->next;
						}
						temp->next = &procTable[iotemp->procid];
						procTable[iotemp->procid].prev = temp;
						procTable[iotemp->procid].next = NULL;	
						procTable[iotemp->procid].state = S_READY;
						readyQueue.len++;

						if (runningProc != &idleProc && runningProc->state == S_RUNNING){
							putReadyCheck = 1;
						}
					}
					else {
						procTable[iotemp->procid].state = S_TERMINATE;
						if (runningProc != &idleProc && runningProc->state == S_RUNNING){
							putReadyCheck = 1;
						}
					}
					if (iotemp->next != NULL){
						iotemp2 = iotemp->next;
						iotemp->prev->next = iotemp->next;
						iotemp->next->prev = iotemp->prev;
						iotemp->prev = iotemp->next = NULL;
						iotemp = iotemp2;
					}
					else{
						iotemp->prev->next = NULL;
						iotemp->next = iotemp->prev = NULL;
					}
					scheCheck = 1;
					ioDoneEventQueue.len--;
					i--;
				}
				else if (iotemp->next != NULL){
					iotemp = iotemp->next;
				}
			}
		}
		if (qTime == QUANTUM) { /* CASE 1 : The quantum expires */
			if (runningProc != &idleProc && runningProc->state == S_RUNNING){
				putReadyCheck = 1;
				runningProc->priority--;
				scheCheck = 1;	
			}
			else if(runningProc != &idleProc && runningProc->state == S_BLOCKED){
				runningProc->priority--;
			}
		}
		if (putReadyCheck == 1){
			putReadyCheck = 0;
			struct process *temp = &readyQueue;     
			while (!(temp->next == NULL || temp->next == &readyQueue)){
				temp = temp->next;
			}
			temp->next = runningProc;
			procTable[runningProc->id].prev = temp;
			procTable[runningProc->id].next = NULL;
			readyQueue.len++;

 			runningProc->state = S_READY;
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
		}
		if (scheCheck == 1){
			qTime = 0;
			scheCheck = 0;
			scheduler();
		}
		pid = runningProc->id;
	} // while loop
}
//RR,SJF(Modified),SRTN,Guaranteed Scheduling(modified),Simple Feed Back Scheduling
struct process* RRschedule() {
	struct process *temp;
	if (readyQueue.len > 0){
		temp = readyQueue.next;		
		if (temp->next != NULL){
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
		}
		else{
			temp->prev->next = NULL;
		}
		temp->next = temp->prev = NULL;
		readyQueue.len--;

		runningProc = &procTable[temp->id];
		procTable[temp->id].state = S_RUNNING;
		cpuReg0 = procTable[temp->id].saveReg0;
		cpuReg1 = procTable[temp->id].saveReg1;
	}
	else{
		runningProc = &idleProc;
	}
}
struct process* SJFschedule() {
	int min = INT_MAX;
	struct process *temp;
	struct process *check = &readyQueue;     
	while (!(check == NULL || check->next == &readyQueue)){ /* mintime check*/
		if (check->targetServiceTime < min && check->targetServiceTime > 0){
			min = check->targetServiceTime;
			temp = check;
		}
		check = check->next;
	}
	
	//check nothing in queue and set runningProc 
	if (min == INT_MAX){
		runningProc = &idleProc;
	}
	else{
		//remove target in readyqueue
		if (temp->next != NULL){
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
		}
		else{
			temp->prev->next = NULL;
		}
		temp->next = temp->prev = NULL;
		readyQueue.len--;

		runningProc = &procTable[temp->id];
		procTable[temp->id].state = S_RUNNING;
		cpuReg0 = procTable[temp->id].saveReg0;
		cpuReg1 = procTable[temp->id].saveReg1;
	}
}
struct process* SRTNschedule() {
	int min = INT_MAX;
	struct process *temp;
	struct process *check = &readyQueue;     
	while (!(check == NULL || check->next == &readyQueue)){ /* mintime check*/
		if ((check->targetServiceTime - check->serviceTime) < min && check->targetServiceTime > 0){
			min = (check->targetServiceTime - check->serviceTime);
			temp = check;
		}
		check = check->next;
	}
	
	//check nothing in queue and set runningProc 
	if (min == INT_MAX){
		runningProc = &idleProc;
	}
	else{
		//remove target in readyqueue
		if (temp->next != NULL){
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
		}
		else{
			temp->prev->next = NULL;
		}
		temp->next = temp->prev = NULL;
		readyQueue.len--;

		runningProc = &procTable[temp->id];
		procTable[temp->id].state = S_RUNNING;
		cpuReg0 = procTable[temp->id].saveReg0;
		cpuReg1 = procTable[temp->id].saveReg1;
	}	
}
struct process* GSschedule() {	
	float min = FLT_MAX;
	struct process *temp;
	struct process *check = &readyQueue;     
	while (!(check == NULL || check->next == &readyQueue)){ /* mintime check*/
		if (((float)check->serviceTime / (float)check->targetServiceTime) < min && check->targetServiceTime > 0){
			min = ((float)check->serviceTime / (float)check->targetServiceTime);
			temp = check;
		}
		check = check->next;
	}
	
	//check nothing in queue and set runningProc 
	if (min == FLT_MAX){
		runningProc = &idleProc;
	}
	else{
		//remove target in readyqueue
		if (temp->next != NULL){
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
		}
		else{
			temp->prev->next = NULL;
		}
		temp->next = temp->prev = NULL;
		readyQueue.len--;

		runningProc = &procTable[temp->id];
		procTable[temp->id].state = S_RUNNING;
		cpuReg0 = procTable[temp->id].saveReg0;
		cpuReg1 = procTable[temp->id].saveReg1;
	}
}
struct process* SFSschedule() {
	int max = INT_MIN;
	struct process *temp;
	struct process *check = &readyQueue;     
	while (!(check == NULL || check->next == &readyQueue)){ /* mintime check*/
		if (check->priority > max && check->targetServiceTime > 0){
			max = check->priority;
			temp = check;
		}
		check = check->next;
	}
	
	//check nothing in queue and set runningProc 
	if (max == INT_MIN){
		runningProc = &idleProc;
	}
	else{
		//remove target in readyqueue
		if (temp->next != NULL){
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
		}
		else{
			temp->prev->next = NULL;
		}
		temp->next = temp->prev = NULL;
		readyQueue.len--;

		runningProc = &procTable[temp->id];
		procTable[temp->id].state = S_RUNNING;
		cpuReg0 = procTable[temp->id].saveReg0;
		cpuReg1 = procTable[temp->id].saveReg1;
	}	
}

void printResult() {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	long totalProcIntArrTime=0,totalProcServTime=0,totalIOReqIntArrTime=0,totalIOServTime=0;
	long totalWallTime=0, totalRegValue=0;
	for(i=0; i < NPROC; i++) {
		totalWallTime += procTable[i].endTime - procTable[i].startTime;
		/*
		printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
			i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);
		*/
		totalRegValue += procTable[i].saveReg0+procTable[i].saveReg1;
		/* printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue);*/
	}
	for(i = 0; i < NPROC; i++) {
		totalProcIntArrTime += procIntArrTime[i];
		totalProcServTime += procServTime[i];
	}
	for(i = 0; i < NIOREQ; i++) {
		totalIOReqIntArrTime += ioReqIntArrTime[i];
		totalIOServTime += ioServTime[i];
	}

	printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float) totalProcIntArrTime/NPROC, (float) totalProcServTime/NPROC);
	printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float) totalIOReqIntArrTime/NIOREQ, (float) totalIOServTime/NIOREQ);
	printf("%d Process processed with %d IO requests\n", NPROC,NIOREQ);
	printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float) totalWallTime/NPROC, (float) totalRegValue/NPROC);

}

int main(int argc, char *argv[]) {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	int totalProcServTime = 0, ioReqAvgIntArrTime;
	int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;
	
	if (argc < 12) {
		printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n",argv[0]);
		exit(1);
	}
	
	SCHEDULING_METHOD = atoi(argv[1]);
	NPROC = atoi(argv[2]);
	NIOREQ = atoi(argv[3]);
	QUANTUM = atoi(argv[4]);
	MIN_INT_ARRTIME = atoi(argv[5]);
	MAX_INT_ARRTIME = atoi(argv[6]);
	MIN_SERVTIME = atoi(argv[7]);
	MAX_SERVTIME = atoi(argv[8]);
	MIN_IO_SERVTIME = atoi(argv[9]);
	MAX_IO_SERVTIME = atoi(argv[10]);
	MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);
	
	printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
	printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
	printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);
	
	srandom(SEED);
	
	// allocate array structures
	procTable = (struct process *) malloc(sizeof(struct process) * NPROC);
	ioDoneEvent = (struct ioDoneEvent *) malloc(sizeof(struct ioDoneEvent) * NIOREQ);
	procIntArrTime = (int *) malloc(sizeof(int) * NPROC);
	procServTime = (int *) malloc(sizeof(int) * NPROC);
	ioReqIntArrTime = (int *) malloc(sizeof(int) * NIOREQ);
	ioServTime = (int *) malloc(sizeof(int) * NIOREQ);

	// initialize queues
	readyQueue.next = readyQueue.prev = &readyQueue;
	
	blockedQueue.next = blockedQueue.prev = &blockedQueue;
	ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
	ioDoneEventQueue.doneTime = INT_MAX;
	ioDoneEventQueue.procid = -1;
	ioDoneEventQueue.len  = readyQueue.len = blockedQueue.len = 0;
	
	// generate process interarrival times
	for(i = 0; i < NPROC; i++ ) { 
		procIntArrTime[i] = random()%(MAX_INT_ARRTIME - MIN_INT_ARRTIME+1) + MIN_INT_ARRTIME;
	}
	
	// assign service time for each process
	for(i=0; i < NPROC; i++) {
		procServTime[i] = random()% (MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
		totalProcServTime += procServTime[i];	
	}
	
	ioReqAvgIntArrTime = totalProcServTime/(NIOREQ+1);
	
	// generate io request interarrival time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioReqIntArrTime[i] = random()%((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+1) + MIN_IOREQ_INT_ARRTIME;
	}
	
	// generate io request service time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioServTime[i] = random()%(MAX_IO_SERVTIME - MIN_IO_SERVTIME+1) + MIN_IO_SERVTIME;
	}
	
#ifdef DEBUG
	// printing process interarrival time and service time
	printf("Process Interarrival Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procIntArrTime[i]);
	}
	printf("\n");
	printf("Process Target Service Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procTable[i].targetServiceTime);
	}
	printf("\n");
#endif
	
	// printing io request interarrival time and io request service time
	printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
	printf("IO Req InterArrival Time range : %d ~ %d\n",MIN_IOREQ_INT_ARRTIME,
			(ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+ MIN_IOREQ_INT_ARRTIME);
			
#ifdef DEBUG		
	printf("IO Req Interarrival Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioReqIntArrTime[i]);
	}
	printf("\n");
	printf("IO Req Service Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioServTime[i]);
	}
	printf("\n");
#endif
	
	struct process* (*schFunc)();
	switch(SCHEDULING_METHOD) {
		case 1 : schFunc = RRschedule; break;
		case 2 : schFunc = SJFschedule; break;
		case 3 : schFunc = SRTNschedule; break;
		case 4 : schFunc = GSschedule; break;
		case 5 : schFunc = SFSschedule; break;
		default : printf("ERROR : Unknown Scheduling Method\n"); exit(1);
	}
	initProcTable();
	procExecSim(schFunc);
	printResult();
}

