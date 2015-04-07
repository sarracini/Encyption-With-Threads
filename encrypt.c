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

// some global variables
int KEY;
int bufSize;
int nIN;
int nOUT;
int nWORK; 
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

void thread_sleep(void){
	struct timespec t;
	int seed = 0;
	t.tv_sec = 0;
	t.tv_nsec = rand_r((unsigned int*)&seed)%(TEN_MILLIS_IN_NANOS+1);
	nanosleep(&t, NULL);
}

void *IN_thread(void){

	thread_sleep();
	pthread_mutex_lock(&mutexIN);
		// critical section for writing to buffer here
	pthread_mutex_unlock(&mutexIN);
	return NULL;

}

void *WORK_thread(void){
	
	thread_sleep();
	pthread_mutex_lock(&mutexWORK);
		// critical section for encrypting/decrypting here
	pthread_mutex_unlock(&mutexWORK);
	return NULL;

}

void *OUT_thread(void){
	
	thread_sleep();
	pthread_mutex_lock(&mutexOUT);
		// critical section for reading from buffer and writing to file here
	pthread_mutex_unlock(&mutexOUT);
	return NULL;

}

int main(int argc, char *argv[]){
	int i;

	// initialize all mutexes
	pthread_mutex_init(&mutexIN, NULL);
	pthread_mutex_init(&mutexWORK, NULL);
	pthread_mutex_init(&mutexOUT, NULL);

	// read in arguments
	char *infile = argv[4];
	char *outfile = argv[5];
	KEY = atoi(argv[0]);
	nIN = atoi(argv[1]);
	nWORK = atoi(argv[2]);
	nOUT = atoi(argv[3]);
	bufSize = atoi(argv[6]);

	// threads
	pthread_t INthreads[nIN];
	pthread_t OUTthreads[nOUT];
	pthread_t WORKthreads[nWORK];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	BufferItem *result = (BufferItem*)malloc(sizeof(BufferItem));

	// reading in to file error handling
	if (!(file_in = fopen(infile, "r"))){
		fprintf(stderr, "could not open input file for reading");
	}

	result->offset = ftell(file_in);
	result->data = fgetc(file_in);

	// writing to file error handling
	if (!(file_out = fopen(outfile, "w"))){
		fprintf(stderr, "could not open output file for writing");
	}
	if (fseek(file_out, result->offset, SEEK_SET) == -1){
		fprintf(stderr, "error setting output file position to %u\n", (unsigned int) result->offset);
		exit(-1);
	}
	if (fputc(result->data, file_out) == EOF){
		fprintf(stderr, "error writing byte %c to output file\n", result->data);
	}

	// create as many in threads as user specified
	for (i = 1; i < nIN; i++){
		pthread_create(&INthreads[i], &attr, (void*) IN_thread, NULL);
	}
	// create as many work threads as user specified
	for (i = 1; i < nWORK; i++){
		pthread_create(&WORKthreads[i], &attr, (void*) WORK_thread, NULL);
	}
	// create as many out threads as user specified
	for (i = 1; i < nOUT; i++){
		pthread_create(&OUTthreads[i], &attr, (void*) OUT_thread, NULL);
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

	return 0;
}