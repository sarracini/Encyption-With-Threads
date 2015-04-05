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

int KEY;
int bufSize;
int nIN;
int nOUT;
int nWORK; 
FILE *file_in;
FILE *file_out;

pthread_mutex_t mutexIN;
pthread_mutex_t mutexWORK;
pthread_mutex_t mutexOUT;

typedef struct{
	char data;
	off_t offset;
	char state;
} BufferItem;

void *IN_thread(void){

	pthread_mutex_lock(&mutexIN);
		// critical section for writing to buffer here

	pthread_mutex_unlock(&mutexIN);
	return NULL;

}

void *WORK_thread(void){
	
	pthread_mutex_lock(&mutexWORK);
		// critical section for encrypting/decrypting here

	pthread_mutex_unlock(&mutexWORK);
	return NULL;

}

void *OUT_thread(void){
	
	pthread_mutex_lock(&mutexOUT);
		// critical section for reading from buffer and writing to file here

	pthread_mutex_unlock(&mutexOUT);
	return NULL;

}

int main(int argc, char *argv[]){
	// this doesn't actually go here i don't know where to put it yet
	struct timespec t;
	int seed = 0;
	t.tv_sec = 0;
	t.tv_nsec = rand_r((unsigned int*)&seed)%(TEN_MILLIS_IN_NANOS+1);

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
	pthread_t INtheads[nIN];
	pthread_t OUTtheads[nOUT];
	pthread_t WORKtheads[nWORK];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	// reading in to file
	if (!(file_in = fopen(infile, "r"))){
		fprintf(stderr, "could not open input file for reading");
	}
	// writing to file
	if (!(file_out = fopen(outfile, "w"))){
		fprintf(stderr, "could not open output file for writing");
	}

	// create as many in threads as user specified
	for (i = 1; i < nIN; i++){
		pthread_create(&INtheads[i], &attr, (void*) IN_thread, NULL;
	}
	// create as many work threads as user specified
	for (i = 1; i < nWORK; i++){
		pthread_create(&WORKtheads[i], &attr, (void*) WORK_thread, NULL);
	}
	// create as many out threads as user specified
	for (i = 1; i < nOUT; i++){
		pthread_create(&OUTtheads[i], &attr, (void*) OUT_thread, NULL);
	}

	// join all the threads
	for (i = 1; i < nIN; i++){
		pthread_join(&INtheads[i], NULL;
	}
	for (i = 1; i < nWORK; i++){
		pthread_join(&WORKtheads[i], NULL);
	}
	for (i = 1; i < nOUT; i++){
		pthread_join(&OUTtheads[i], NULL);
	}

	// destory all mutexes
	pthread_mutex_destroy(&mutexIN);
	pthread_mutex_destroy(&mutexOUT);
	pthread_mutex_destroy(&mutexWORK);

	return 0;
}