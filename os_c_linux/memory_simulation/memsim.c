//
// Operaitng System
// Virtual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// make by Park Dongjun
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes

int PHYMEMCOUNT = 0;
int SECONDPAGESIZE = 0;
int HASHLINKEDSIZE = 0;
int nFrame;

struct phyMem {
	int phyMemid;
	int pageTableid;
	int pageTableid2;
	int pid;
	int len;
	int maxLen;
	struct phyMem *next;
	struct phyMem *prev;
	struct phyMem *tail;
} phyMemQueue, *phyMem;

struct hashTableEntry {
	int hashTableid;
	int phyMemid;
	int vpn;
	int pid;
	struct hashTableEntry *next;
} *hashTable, *hashLinked;

struct pageTableEntry {
	int pageTableid;
	int phyMemid;
	int vaild;
	struct pageTableEntry *second;
} **pageTable, **secondPageTable;

struct procEntry {
	char *traceName;			// the memory trace name
	int pid;				// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;		// The 2nd level page created(allocated);
	int numIHTConflictAccess; 		// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;			// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAccess;		// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	struct pageTableEntry *firstLevelPageTable;
	FILE *tracefp;
} *procTable;

void oneLevelVMSim(int numProcess, int howRepl) {
	int i, j;

	do{
		i = 0;
		for(j = 0; j < numProcess; j++) {
			unsigned addr;
			char rw;
			if((fscanf(procTable[j].tracefp, "%x %c", &addr,&rw)) == EOF){
				i++;
			}
			else{
				unsigned vpn = addr>>PAGESIZEBITS;
				if(howRepl == 0){     //FIFO
					if(procTable[j].firstLevelPageTable[vpn].vaild == 1){
						procTable[j].numPageHit++;
					}
					else{
						procTable[j].numPageFault++;
						if(phyMemQueue.len < phyMemQueue.maxLen){
							phyMemQueue.tail->next = &phyMem[PHYMEMCOUNT];
							phyMemQueue.tail = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].pageTableid = vpn;
							phyMem[PHYMEMCOUNT].pid = j;
							procTable[j].firstLevelPageTable[vpn].vaild = 1;
							phyMemQueue.len++;
							if(PHYMEMCOUNT == (phyMemQueue.maxLen - 1)){
								PHYMEMCOUNT = 0;
							}
							else{
								PHYMEMCOUNT++;
							}
						}
						else{
							struct phyMem *temp = phyMemQueue.next;
							if(phyMemQueue.next->next != NULL){
								phyMemQueue.next = phyMemQueue.next->next;
							}
							temp->next = NULL;
							procTable[phyMem[PHYMEMCOUNT].pid].firstLevelPageTable[phyMem[PHYMEMCOUNT].pageTableid].vaild = 0;

							phyMemQueue.tail->next = &phyMem[PHYMEMCOUNT];
							phyMemQueue.tail = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].pageTableid = vpn;
							phyMem[PHYMEMCOUNT].pid = j;
							procTable[j].firstLevelPageTable[vpn].vaild = 1;
							if(PHYMEMCOUNT == (phyMemQueue.maxLen - 1)){
								PHYMEMCOUNT = 0;
							}
							else{
								PHYMEMCOUNT++;
							}
						}	
					}
				}
				else{		//LRU
					if(procTable[j].firstLevelPageTable[vpn].vaild == 1){
						procTable[j].numPageHit++;
						struct phyMem *temp = &phyMem[procTable[j].firstLevelPageTable[vpn].phyMemid];
						if(temp->next != NULL){
							temp->prev->next = temp->next;
							temp->next->prev = temp->prev;
							temp->next = NULL;
							temp->prev = NULL;
							phyMemQueue.tail->next = temp;
							temp->prev = phyMemQueue.tail;
							phyMemQueue.tail = temp;
						}
					}
					else{
						procTable[j].numPageFault++;
						if(phyMemQueue.len < phyMemQueue.maxLen){
							phyMemQueue.tail->next = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].prev = phyMemQueue.tail;
							phyMemQueue.tail = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].pageTableid = vpn;
							phyMem[PHYMEMCOUNT].pid = j;
							procTable[j].firstLevelPageTable[vpn].vaild = 1;
							procTable[j].firstLevelPageTable[vpn].phyMemid = PHYMEMCOUNT;
							phyMemQueue.len++;
							PHYMEMCOUNT++;
						}
						else{
							struct phyMem *temp = phyMemQueue.next;
							if(phyMemQueue.next->next != NULL){
								phyMemQueue.next = phyMemQueue.next->next;
								temp->next->prev = temp->prev;
							}
							temp->next = NULL;
							temp->prev = NULL;
							procTable[temp->pid].firstLevelPageTable[temp->pageTableid].vaild = 0;

							if(phyMemQueue.next->next != NULL){
								phyMemQueue.tail->next = temp;
								temp->prev = phyMemQueue.tail;
								phyMemQueue.tail = temp;
							}
							temp->pageTableid = vpn;
							temp->pid = j;
							procTable[j].firstLevelPageTable[vpn].vaild = 1;
							procTable[j].firstLevelPageTable[vpn].phyMemid = temp->phyMemid;
						}	
					}
				}
				procTable[j].ntraces++;
			}
		}
	} while(i < 1);

	for(i = 0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
	
}
void twoLevelVMSim(int numProcess, int firstLevelBits) {
	int i, j;
		
	do{
		i = 0;
		for(j = 0; j < numProcess; j++) {
			unsigned addr;
			char rw;
			if((fscanf(procTable[j].tracefp, "%x %c", &addr,&rw)) == EOF){
				i++;
			}
			else{
				unsigned fVpn = addr>>(VIRTUALADDRBITS - firstLevelBits);
				unsigned sVpn = (addr<<firstLevelBits)>>(PAGESIZEBITS + firstLevelBits);
				
				if(procTable[j].firstLevelPageTable[fVpn].second != NULL){
					struct pageTableEntry *temp = procTable[j].firstLevelPageTable[fVpn].second;
					if(temp[sVpn].vaild == 1){
						procTable[j].numPageHit++;
						struct phyMem *temp = &phyMem[procTable[j].firstLevelPageTable[fVpn].second[sVpn].phyMemid];
						if(temp->next != NULL){
							temp->prev->next = temp->next;
							temp->next->prev = temp->prev;
							temp->next = NULL;
							temp->prev = NULL;
							phyMemQueue.tail->next = temp;
							temp->prev = phyMemQueue.tail;
							phyMemQueue.tail = temp;
						}
					}
					else{
						procTable[j].numPageFault++;
						if(phyMemQueue.len < phyMemQueue.maxLen){
							phyMemQueue.tail->next = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].prev = phyMemQueue.tail;
							phyMemQueue.tail = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].pageTableid = fVpn;
							phyMem[PHYMEMCOUNT].pageTableid2 = sVpn;
							phyMem[PHYMEMCOUNT].pid = j;
							procTable[j].firstLevelPageTable[fVpn].second[sVpn].vaild = 1;
							procTable[j].firstLevelPageTable[fVpn].second[sVpn].phyMemid = PHYMEMCOUNT;
							phyMemQueue.len++;
							PHYMEMCOUNT++;
						}
						else{
							struct phyMem *temp = phyMemQueue.next;
							if(phyMemQueue.next->next != NULL){
								phyMemQueue.next = phyMemQueue.next->next;
								temp->next->prev = temp->prev;
							}
							temp->next = NULL;
							temp->prev = NULL;
							procTable[temp->pid].firstLevelPageTable[temp->pageTableid].second[temp->pageTableid2].vaild = 0;

							if(phyMemQueue.next->next != NULL){
								phyMemQueue.tail->next = temp;
								temp->prev = phyMemQueue.tail;
								phyMemQueue.tail = temp;
							}
							temp->pageTableid = fVpn;
							temp->pageTableid2 = sVpn;
							temp->pid = j;
							procTable[j].firstLevelPageTable[fVpn].second[sVpn].vaild = 1;
							procTable[j].firstLevelPageTable[fVpn].second[sVpn].phyMemid = temp->phyMemid;
						}
					}
				}
				else{
					//realloc
					SECONDPAGESIZE++;
					secondPageTable = (struct pageTableEntry **)realloc(secondPageTable, sizeof(struct pageTableEntry *) * (SECONDPAGESIZE + 1));
					secondPageTable[SECONDPAGESIZE - 1] = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1L<<(VIRTUALADDRBITS - PAGESIZEBITS - firstLevelBits)));
					//connect pagetable
					procTable[j].firstLevelPageTable[fVpn].second = secondPageTable[SECONDPAGESIZE - 1];
					procTable[j].num2ndLevelPageTable++;	

					procTable[j].numPageFault++;
					if(phyMemQueue.len < phyMemQueue.maxLen){
						phyMemQueue.tail->next = &phyMem[PHYMEMCOUNT];
						phyMem[PHYMEMCOUNT].prev = phyMemQueue.tail;
						phyMemQueue.tail = &phyMem[PHYMEMCOUNT];
						phyMem[PHYMEMCOUNT].pageTableid = fVpn;
						phyMem[PHYMEMCOUNT].pageTableid2 = sVpn;
						phyMem[PHYMEMCOUNT].pid = j;
						procTable[j].firstLevelPageTable[fVpn].second[sVpn].vaild = 1;
						procTable[j].firstLevelPageTable[fVpn].second[sVpn].phyMemid = PHYMEMCOUNT;
						phyMemQueue.len++;
						PHYMEMCOUNT++;
					}
					else{
						struct phyMem *temp = phyMemQueue.next;
						if(phyMemQueue.next->next != NULL){
							phyMemQueue.next = temp->next;
							temp->next->prev = temp->prev;
						}
						temp->next = NULL;
						temp->prev = NULL;
						procTable[temp->pid].firstLevelPageTable[temp->pageTableid].second[temp->pageTableid2].vaild = 0;
						
						if(phyMemQueue.next->next != NULL){
							phyMemQueue.tail->next = temp;
							temp->prev = phyMemQueue.tail;
							phyMemQueue.tail = temp;
						}
						temp->pageTableid = fVpn;
						temp->pageTableid2 = sVpn;
						temp->pid = j;
						procTable[j].firstLevelPageTable[fVpn].second[sVpn].vaild = 1;
						procTable[j].firstLevelPageTable[fVpn].second[sVpn].phyMemid = temp->phyMemid;
					}
				}	
				procTable[j].ntraces++;
			}
		}
	} while(i < 1);

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void invertedPageVMSim(int numProcess) {
	int i, j;
	
	hashTable = (struct hashTableEntry *)malloc(sizeof(struct hashTableEntry) * (1L<<nFrame));
	hashLinked = (struct hashTableEntry *)malloc(sizeof(struct hashTableEntry) * (1L<<nFrame));
	
	for(i = 0; i < (1L<<nFrame); i++){
		hashTable[i].hashTableid = i;
		hashTable[i].next = NULL;
		hashLinked[i].phyMemid = -1;
		hashLinked[i].vpn = -1;
		hashLinked[i].pid = -1;
		hashLinked[i].next = NULL;
	}

	do{
		i = 0;
		for(j = 0; j < numProcess; j++) {
			unsigned addr;
			char rw;
			if((fscanf(procTable[j].tracefp, "%x %c", &addr,&rw)) == EOF){
				i++;
			}
			else{
				unsigned vpn = addr>>PAGESIZEBITS;
				int hashIndex = (vpn + j) % (1L<<nFrame);
				if(hashTable[hashIndex].next == NULL){
					procTable[j].numIHTNULLAccess++;
					procTable[j].numPageFault++;
					
					//create new linked
					if(phyMemQueue.len < phyMemQueue.maxLen){
						phyMemQueue.tail->next = &phyMem[PHYMEMCOUNT];
						phyMem[PHYMEMCOUNT].prev = phyMemQueue.tail;
						phyMemQueue.tail = &phyMem[PHYMEMCOUNT];
						phyMem[PHYMEMCOUNT].pageTableid = vpn;
						phyMem[PHYMEMCOUNT].pid = j;

						hashTable[hashIndex].next = &hashLinked[HASHLINKEDSIZE];
						hashLinked[HASHLINKEDSIZE].pid = j;
						hashLinked[HASHLINKEDSIZE].vpn = vpn;
						hashLinked[HASHLINKEDSIZE].hashTableid = HASHLINKEDSIZE;
				 		hashLinked[HASHLINKEDSIZE].phyMemid = PHYMEMCOUNT;
						HASHLINKEDSIZE++;

						phyMemQueue.len++;
						if(PHYMEMCOUNT == (phyMemQueue.maxLen - 1)){
							PHYMEMCOUNT = 0;
						}
						else{
							PHYMEMCOUNT++;
						}
					}
					else{
						struct phyMem *temp = phyMemQueue.next;
						if(phyMemQueue.next->next != NULL){
							phyMemQueue.next = phyMemQueue.next->next;
							temp->next->prev = temp->prev;
						}
						temp->next = NULL;
						temp->prev = NULL;
						
						//hashtable find it delete
						int tempHash = (temp->pageTableid + temp->pid) % (1L<<nFrame);
						struct hashTableEntry * htemp = &hashTable[tempHash];
						struct hashTableEntry * htemp2;
						while(htemp->next->vpn != temp->pageTableid || htemp->next->pid != temp->pid){
							htemp = htemp->next;
						}
						htemp2 = htemp->next;
						htemp->next = htemp2->next;
						htemp2->next = NULL;

						phyMemQueue.tail->next = temp;
						temp->prev = phyMemQueue.tail;
						phyMemQueue.tail = temp;
						temp->pageTableid = vpn;
						temp->pid = j;

						hashTable[hashIndex].next = htemp2;
						htemp2->pid = j;
						htemp2->vpn = vpn;
					}
				}
				else{
					procTable[j].numIHTNonNULLAccess++;

					//follow the linked 
					int inCheck = 1;
					struct hashTableEntry *htemp = &hashTable[hashIndex];
					while(htemp->next->vpn != vpn || htemp->next->pid != j){
						procTable[j].numIHTConflictAccess++;
						if(htemp->next->next == NULL){
							inCheck = 0;
							break;
						}
						htemp = htemp->next;
					}
					if(inCheck == 1){	
						procTable[j].numIHTConflictAccess++;
						procTable[j].numPageHit++;
						struct phyMem *temp = &phyMem[htemp->next->phyMemid];
						if(temp->next != NULL){
							temp->prev->next = temp->next;
							temp->next->prev = temp->prev;
						}
						temp->next = NULL;
						temp->prev = NULL;
						phyMemQueue.tail->next = temp;
						temp->prev = phyMemQueue.tail;
						phyMemQueue.tail = temp;
					}
					else{
						procTable[j].numPageFault++;
						if(phyMemQueue.len < phyMemQueue.maxLen){
							phyMemQueue.tail->next = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].prev = phyMemQueue.tail;
							phyMemQueue.tail = &phyMem[PHYMEMCOUNT];
							phyMem[PHYMEMCOUNT].pageTableid = vpn;
							phyMem[PHYMEMCOUNT].pid = j;

							hashLinked[HASHLINKEDSIZE].next = hashTable[hashIndex].next;
							hashTable[hashIndex].next = &hashLinked[HASHLINKEDSIZE];
							hashLinked[HASHLINKEDSIZE].pid = j;
							hashLinked[HASHLINKEDSIZE].vpn = vpn;
							hashLinked[HASHLINKEDSIZE].hashTableid = HASHLINKEDSIZE;
					 		hashLinked[HASHLINKEDSIZE].phyMemid = PHYMEMCOUNT;
							HASHLINKEDSIZE++;

							phyMemQueue.len++;
							PHYMEMCOUNT++;
						}
						else{
							struct phyMem *temp = phyMemQueue.next;
							if(phyMemQueue.next->next != NULL){
								phyMemQueue.next = phyMemQueue.next->next;
								temp->next->prev = temp->prev;
							}
							temp->next = NULL;
							temp->prev = NULL;

							//hashtable find it delete
							int tempHash = (temp->pageTableid + temp->pid) % (1L<<nFrame);
							struct hashTableEntry * htemp = &hashTable[tempHash];
							struct hashTableEntry * htemp2;
							while(htemp->next->vpn != temp->pageTableid || htemp->next->pid != temp->pid){
								htemp = htemp->next;
							}
							htemp2 = htemp->next;
							htemp->next = htemp2->next;
							htemp2->next = NULL;
							
							phyMemQueue.tail->next = temp;
							temp->prev = phyMemQueue.tail;
							phyMemQueue.tail = temp;
							temp->pageTableid = vpn;
							temp->pid = j;

							htemp2->next = hashTable[hashIndex].next;
							hashTable[hashIndex].next = htemp2;
							htemp2->pid = j;
							htemp2->vpn = vpn;
						}
					}
				}
				procTable[j].ntraces++;
			}
		}
	} while(i < 1);

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAccess);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAccess == procTable[i].ntraces);
	}
}

int main(int argc, char *argv[]) {
	int i, j, simType;
	int firstLevelBits, phyMemSizeBits;
	int secondLevelBits;
	int numProcess = 0;
	
	simType = atoi(argv[1]);
	firstLevelBits = atoi(argv[2]);
	phyMemSizeBits = atoi(argv[3]);
	nFrame = phyMemSizeBits - PAGESIZEBITS;

	//count process
	numProcess = argc - 4;
	
	//error check
	if(argv[1][0] == '-'){
		printf("ERROR: Option is not supported\n");
		exit(1);
	}
	if(simType < 0 || simType > 3){
		printf("ERROR: Wrong SimType\n");
		exit(1);
	}	
	if(firstLevelBits >= (VIRTUALADDRBITS - PAGESIZEBITS) || firstLevelBits < 1){
		printf("ERROR: Wrong FirstLevelBits\n");
		exit(1);
	}
	if(phyMemSizeBits < PAGESIZEBITS){
		printf("ERROR: Too small PhysicalMemorySizeBits\n");
		exit(1);
	}
	if(numProcess < 1) {
		printf("ERROR: There are no process\n");
		exit(1);
	}

	if(simType != 2){
		secondLevelBits = 0;
	}
	else{
		secondLevelBits = VIRTUALADDRBITS - PAGESIZEBITS - firstLevelBits;
	}
	
	// memory allocate		
	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess);
	phyMem = (struct phyMem *)malloc(sizeof(struct phyMem) * (1L<<(phyMemSizeBits - PAGESIZEBITS)));
	pageTable = (struct pageTableEntry **)malloc(sizeof(struct pageTableEntry *) * numProcess);
	if(simType == 2){
		secondPageTable = (struct pageTableEntry **)malloc(sizeof(struct pageTableEntry *) * 1);
	}

	//initialize
	phyMemQueue.next = phyMemQueue.tail = phyMemQueue.prev = &phyMemQueue;
	phyMemQueue.len = 0;
	phyMemQueue.maxLen = 1L<<(phyMemSizeBits - PAGESIZEBITS);

	for(i = 0; i < (1L<<(phyMemSizeBits - PAGESIZEBITS)); i++){
		phyMem[i].phyMemid = i;
		phyMem[i].pageTableid = -1;
		phyMem[i].pageTableid = -1;
		phyMem[i].pid = -1;
		phyMem[i].next = NULL;
	}

	// initialize procTable for Memory Simulations
	for(i = 0; i < numProcess; i++) {
		procTable[i].pid = i;
		procTable[i].ntraces = 0;	
		procTable[i].num2ndLevelPageTable = 0;	
		procTable[i].numIHTConflictAccess = 0; 
		procTable[i].numIHTNULLAccess = 0;
		procTable[i].numIHTNonNULLAccess = 0;	
		procTable[i].numPageFault = 0;
		procTable[i].numPageHit = 0;
		
		//pagetable create and connect
		pageTable[i] = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1L<<(VIRTUALADDRBITS - PAGESIZEBITS - secondLevelBits)));
		
		for(j = 0; j < (1L<<(VIRTUALADDRBITS - PAGESIZEBITS - secondLevelBits)); j++){
			pageTable[i][j].vaild = 0;
			pageTable[i][j].pageTableid = j;
			pageTable[i][j].phyMemid = -1;
			pageTable[i][j].second = NULL;
		}
	
		procTable[i].firstLevelPageTable = pageTable[i];
	
		// opening a tracefile for the process
		printf("process %d opening %s\n",i,argv[i + 4]);
		procTable[i].tracefp = fopen(argv[i + 4],"r");
		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...",argv[i + 4]);
			exit(1);
		}
		procTable[i].traceName = argv[i + 4];	
	}

	printf("Num of Frames %d Physical Memory Size %ld bytes\n", (1L<<nFrame), (1L<<phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(numProcess, 0);
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(numProcess, 1);
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		twoLevelVMSim(numProcess, firstLevelBits);
	}
	
	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		invertedPageVMSim(numProcess);
	}

	return(0);
}
