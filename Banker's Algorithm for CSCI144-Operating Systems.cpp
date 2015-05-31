// Christopher Hays
// CSCI 144
// Banker's Algorithm problem
//
// A thread makes continuous random requests for resources and the main thread runs the
//   banker's algorithm to determine if the current request will leave the system in a safe state.
//   If not, the request is rejected.  If four rejections occur in a row, the allocation table
//   is cleared and the program continues. Semaphores are used to isolate the critical section.  
//
// added -pthread to the compile and build command lines, as well as -std=c++11


#include <iostream>			// cout
#include <stdlib.h>			// exit()
#include <pthread.h>		// pthreads
#include <stdbool.h>		// boolean variables
#include <unistd.h>			// fork() and sleep()
#include <semaphore.h>		// semaphores
using namespace std;


// ************ Global Data *****************

#define n 3		// number of processes
#define m 5		// number of resources

sem_t s;		// semaphore controls main thread
sem_t t;		// semaphore controls request thread

struct state{
	int C[m] = 		   {4, 4, 5, 2, 3};    // total resources of the system
	
	// max claim of each process
	int maxc[n][m]  = {{3, 2, 1, 0, 2},    // p0
                       {3, 2, 0, 2, 1},    // p1
                       {4, 2, 2, 0, 3}};   // p2 
                      
    // current allocation table
	int alloc[n][m] = {{0, 0, 0, 0, 0},    // p0
                       {0, 0, 0, 0, 0},    // p1
                       {0, 0, 0, 0, 0}};   // p2
                       
	bool safe = false;		// current state status
	int p;					// process number
	int r;					// resource number
	int count = 0;			// counts rejected requests
} current;


// *********** Function Definitions **************

void clearAlloc(){  // clears the allocation table to avoid deadlock
	for (int i = 0; i < n; i++){
		for (int j = 0; j < m; j++){
			current.alloc[i][j] = 0;
		}
	}
}

void print(int array[][m]){  // prints a 2 dimensional array
	for (int i = 0; i < n; i++){
		for (int j = 0; j < m; j++){
			cout << array[i][j] << " ";
		}
		cout << endl;
	}
}

void print(int array[m]){  // prints an array
	for (int i = 0; i < m; i++){
		cout << array[i] << " ";
	}
}

int check(int need[][m], int avail[m], int P[n]){  // returns the process number if the process has enough resources available to run
	for (int i = 0; i < n; i++){
		if (P[i]){ 								// if the process flag is true
			for (int j = 0; j < m; j++){
				if (need[i][j] > avail[j]){		// if the need is greater than what's available for any given resource,
					break;						//   break and test the next process if there is one 
				}
				if (j == m-1){					// if we pass the last resource check for the process,
					return i;					//    return the number of the process
				}
			}
		}   
    }
    return -1;  // if we end up here then no processes can run
}

bool zeroCheck(int array[][m]){  // check to see if a matrix is all zeros
	for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            if (array[i][j] != 0){
				 return false;
			}
        }   
    }
    return true;
}

bool bankers(int alloc[][m], int C[m], int maxc[][m]){	// returns true if the system is safe with the current allocation and maximum requests
	
	int process = 99;					// current process number
	bool safe = false;					// state of the machine
	
	int alloc_[n][m];					// copy of allocation table
    int sum[m] = {0, 0, 0, 0, 0};		// sum of allocation columns
    int avail[m];						// avail vector
    int need[n][m];						// resources needed to run
    
    // step 1 (copy alloc)
	int P[n] = {1, 1, 1};					// a vector of process flags, set to zero when a particular process is done
    for (int i = 0; i < n; i++){			// make a copy of the alloc matrix
        for (int j = 0; j < m; j++){
            alloc_[i][j] = alloc[i][j];
        }
    }
    
    while (true){  // infinite loop for steps 2 through 4
		
		// step 2 (compute avail)
		for (int i = 0; i < n; i++){ 		// sum the columns of alloc_
			for (int j = 0; j < m; j++){
				sum[j] += alloc_[i][j];
			}
		}
    
		for (int i = 0; i < m; i++){		// compute avail (C - sum)
			avail[i] = C[i] - sum[i];
		}
		 
    
		// step 3 (find a process that will have enough resources)
		for (int i = 0; i < n; i++){
			for (int j = 0; j < m; j++){						// compute process need
				need[i][j] = maxc[i][j] - alloc_[i][j];
			}
		}
    
		process = check(need, avail, P);		// obtain the number of a process that has enough resources to run
		
		if (process == -1){		// if no processes can run, halt the algorithm
			return false;
		}
    
    
		// step 4
		for (int j = 0; j < m; j++){	// remove the completed process' resources from the allocation table
			alloc_[process][j] = 0;		// set them to zero
		}
				
		safe = zeroCheck(alloc_);		// check to see if the allocation table is all zeros
		
		if (safe){
			return true;				// return true if the allocation table is empty
		}
		
		P[process] = 0;					// set the completed process flag to 0
		
		for (int i = 0; i < m; i++){ 	// clear sum vector so the loop can continue
			sum[i] = 0;
		}	
	
	}  // end while
}

// the request thread calls this function
void* functionA(void* arg){  // pointer return type, takes a void pointer as an argument (can point to any data type)
	
	state *current = (state*)(arg);		// cast the passed argument as a state struct
	int p;		// process
	int r;		// resource
	
	while (true){	// infinite loop
		
		sem_wait(&t);  // wait for the semaphore
		sleep(4);	   // wait 4 seconds so the output can be read
		
		do{								// choose a random process and random resource to request
			current->p = rand()%3;      // between 0 and 2
			current->r = rand()%5;      // between 0 and 4
			p = current->p;				// temporary variables for readability
			r = current->r;		
		}while (current->alloc[p][r] >= current->maxc[p][r]);	// loop until the new process and resource are less than the max claim
                                                                // this ensures that the max claim will not be exceeded
		
		current->alloc[p][r] += 1;	// increment the random process/resource on the allocation table
		cout << endl << "Process " << p << " requesting more of resource " << r << " ." << endl; 	
			
		sem_post(&s);	// let the main thread run
	}
	return NULL;
} // end


// ********************   Main   ***********************

int main(){

	sem_init (&s, 1, -1);	// initialize the semaphores 
	sem_init (&t, 1, -1);
	pthread_t processA;  	// an abstract data type used to handle a reference to the thread
	pthread_create(&processA, NULL, functionA, &current);	// (thread handle, default settings, function to begin execution, an argument passed by pointer)
    
    sem_post(&t);	// the request thread will run first
    
    while (true){  // infinite loop
		
		sem_wait(&s);  // wait for the semaphore
	
		sleep(0);  // for testing
	
		cout << endl << "Allocation table: " << endl;	// display allocation table
		print(current.alloc);
		
		current.safe = bankers(current.alloc, current.C, current.maxc);	// run banker's algorithm
		
		cout << endl << "Current request is: "; // print result
		if (current.safe){						// if safe 
			current.count = 0;					// reset rejected counter to zero
			cout << "Accepted." << endl;
		}
		else{											// if unsafe
			current.count += 1;							// increment rejected counter
			current.alloc[current.p][current.r] -= 1;	// decrement the resource that caused the rejection (restore the alloc table)
			cout << "Rejected." << endl;
			if (current.count >= 4){					// if there have been 4 rejections in a row
				clearAlloc();							// clear the allocation table to avoid deadlock
				cout << endl << "Allocation table cleared." << endl;
			}
		}
		
		cout << endl << endl << "* * * * * * * * * * * * * * *" << endl << endl;
		
		sem_post(&t);  // let the request thread run
	}
	
	pthread_join(processA, NULL);	// end the thread
	sem_destroy(&s);				// destroy the semaphore
    sem_destroy(&t);
	
}  // end main


