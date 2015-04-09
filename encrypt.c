/*
Family Name: Sarracini
Given Name: Ursula
Section: Z
Student Number: 211535432
CS Login: cse13208
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define TEN_MILLIS_IN_NANOS 10000000
/* to do:
 -init buffer -> set states to empty
 -functions to retrieve states of buffer
 -ALL the error handling
*/
// some global variables
volatile int nIN = 0;
volatile int nOUT = 0;
volatile int nWORK = 0; 
void *KEY;
int bufSize = 0;
FILE *file_in;
FILE *file_out;

// mutex locks for threads
pthread_mutex_t mutexIN;
pthread_mutex_t mutexWORK;
pthread_mutex_t mutexOUT;

// single item character in buffer
typedef struct{
	char data;
	off_t offset;
	char state;
} BufferItem;

BufferItem *result;

void thread_sleep(void){
	struct timespec t;
	int seed = 0;
	t.tv_sec = 0;
	t.tv_nsec = rand_r((unsigned int*)&seed)%(TEN_MILLIS_IN_NANOS+1);
	nanosleep(&t, NULL);
}

int is_buffer_empty(){
	int i = 0;
	while (i < bufSize){
		if(result[i].state != 'e'){
			return 0;
		}
		i++;
	}
	return 1;
}

void initialize_buffer(){
	int i = 0;
	while (i < bufSize){
		result[i].state = 'e';
		i++;
	}
}

void *IN_thread(void){

	thread_sleep();
	pthread_mutex_lock(&mutexIN);
		// critical section for reading in file
	pthread_mutex_unlock(&mutexIN);

	pthread_mutex_lock(&mutexWORK);
		// critical section for writing to buffer
	pthread_mutex_unlock(&mutexWORK);
	return NULL;

}

void *WORK_thread(void *param){
	int index = 0;
	int key = (int)param;
	
	char curr = result[index].data;

	thread_sleep();
	pthread_mutex_lock(&mutexWORK);
		// critical section for encrypting/decrypting here
	if (key >= 0 && curr > 31 && curr < 127){
		curr = (((int)curr-32)+2*95+key)%95+32;
	}
	else if (key < 0 && curr > 31 && curr < 127){
		curr = (((int)curr-32)+2*95-key)%95+32;
	}
	pthread_mutex_unlock(&mutexWORK);
	return NULL;

}

void *OUT_thread(void){
	
	thread_sleep();
	pthread_mutex_lock(&mutexWORK);
		//critical section for reading from buffer
	pthread_mutex_unlock(&mutexWORK);

	pthread_mutex_lock(&mutexOUT);
		// critical section for writing to file here
	pthread_mutex_unlock(&mutexOUT);
	return NULL;

}

int main(int argc, char *argv[]){
	int i = 0;

	// initialize all mutexes
	pthread_mutex_init(&mutexIN, NULL);
	pthread_mutex_init(&mutexWORK, NULL);
	pthread_mutex_init(&mutexOUT, NULL);

	// read in arguments
	file_in = fopen(argv[5], "r");
	file_out = fopen(argv[6], "w");
	KEY = argv[1];
	nIN = atoi(argv[2]);
	nWORK = atoi(argv[3]);
	nOUT = atoi(argv[4]);
	bufSize = atoi(argv[7]);

	// threads
	pthread_t INthreads[nIN];
	pthread_t OUTthreads[nOUT];
	pthread_t WORKthreads[nWORK];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	result = (BufferItem*)malloc(sizeof(BufferItem));

	// check that the key is valid
	int keyCheck = atoi(KEY);
	if (keyCheck > 127 || keyCheck < -127 ) {
		fprintf(stderr, "enter a valid integer as the key");
		exit(-1);
	}
	// create as many in threads as user specified
	for (i = 1; i < nIN; i++){
		pthread_create(&INthreads[i], &attr, (void *) IN_thread, NULL);
	}
	// create as many work threads as user specified
	for (i = 1; i < nWORK; i++){
		pthread_create(&WORKthreads[i], &attr, (void *) WORK_thread, KEY);
	}
	// create as many out threads as user specified
	for (i = 1; i < nOUT; i++){
		pthread_create(&OUTthreads[i], &attr, (void *) OUT_thread, NULL);
	}

	// join all the threads
	for (i = 1; i < nIN; i++){
		pthread_join(INthreads[i], NULL);
	}
	for (i = 1; i < nWORK; i++){
		pthread_join(WORKthreads[i], NULL);
	}
	for (i = 1; i < nOUT; i++){
		pthread_join(OUTthreads[i], NULL);
	}

	// destory all mutexes
	pthread_mutex_destroy(&mutexIN);
	pthread_mutex_destroy(&mutexOUT);
	pthread_mutex_destroy(&mutexWORK);

	fclose(file_in);
	fclose(file_out);
	return 0;
}