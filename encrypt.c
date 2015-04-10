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
 -do out thread work 
 -ALL the error handling
 -test test test
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
		if(result[i].state == 'e'){
			return 1;
		}
		i++;
	}
	return 0;
}

int first_empty_item_in_buffer(){
	int first_empty;
	int i = 0;
	if (is_buffer_empty()){
		while (i < bufSize){
			if (result[i].state == 'e'){
				first_empty = i;
			}
			i++;
		}
	} else {
		first_empty = -1;
	}
	return first_empty;
}

int first_work_item_in_buffer(){
	int i = 0;
	while (i < bufSize){
		if (result[i].state == 'w'){
			return i;
		}
		i++;
	}
	return -1;
}

int first_out_item_in_buffer(){
	int i = 0;
	while (i < bufSize){
		if (result[i].state == 'o'){
			return i;
		}
		i++;
	}
	return -1;
}

void initialize_buffer(){
	int i = 0;
	while (i < bufSize){
		result[i].state = 'e';
		i++;
	}
}

void valid_input(int param, int expected, char* msg){
	if (param < expected){
		fprintf(stderr, "%s\n", msg);
		exit(-1);
	}
}

void valid_key(int param, char* msg){
	if (param > 127 || param < -127){
		fprintf(stderr, "%s\n", msg);
	}
}

void *IN_thread(void *param){
	int index = 0;

	thread_sleep();

	while (!feof(file_in)){

		// critical section to read in file 
		pthread_mutex_lock(&mutexIN);
		off_t off = ftell(file_in);
		char curr = fgetc(file_in);
		pthread_mutex_unlock(&mutexIN);

		// if reached end of file, produce and error
		if (curr == EOF){
			fprintf(stderr, "error while reading file");
			break;
		}

		// critical section for writing characer to buffer
		pthread_mutex_lock(&mutexWORK);
		index = first_empty_item_in_buffer();
		
		// if buffer is empty, sleep and try again after some time
		if (!is_buffer_empty()){
			thread_sleep();
		}
		if (index != -1){ 
			result[index].offset = off;
			result[index].data = curr;
			result[index].state = 'w';
		}
		pthread_mutex_unlock(&mutexWORK);
		thread_sleep();	
	}
	return NULL;
}

void *WORK_thread(void *param){
	int index = 0;
	int key = (int)param;
	thread_sleep();
	char curr;
	
	// critical section to read from buffer and save current character
	pthread_mutex_lock(&mutexWORK);
	index = first_work_item_in_buffer();
	curr = result[index].data;
	printf("index: %d\n", index);
	pthread_mutex_unlock(&mutexWORK);	

		// encrypting/decrypting file if there is work to be done and buffer is non-empty
		while (index != -1){
			if (key >= 0 && curr > 31 && curr < 127){
				curr = (((int)curr-32)+2*95+key)%95+32;
			}
			else if (key < 0 && curr > 31 && curr < 127){
				curr = (((int)curr-32)+2*95-key)%95+32;
			}
			// if buffer is empty, sleep and try again after some time
			if (is_buffer_empty()){
				thread_sleep();
			}
			
			// critical section to write encrypted character back to buffer, change state and grab next work byte
			pthread_mutex_lock(&mutexWORK);
			result[index].data = curr;
			result[index].state = 'o';
			pthread_mutex_unlock(&mutexWORK);
			thread_sleep();
		}
	
	return NULL;

}

void *OUT_thread(void *param){
	int index = 0;

	thread_sleep();
	
	while (!is_buffer_empty()){
		//critical section for reading from buffer
		pthread_mutex_lock(&mutexWORK);
		index = first_out_item_in_buffer();
		pthread_mutex_unlock(&mutexWORK);
		
		// critical section for writing to file 
		pthread_mutex_lock(&mutexOUT);
		if (!feof(file_out)) {
   			fprintf(stderr, "could not open output file for writing");
		}
		if (fseek(file_out, result[index].offset, SEEK_SET) == -1) {
    		fprintf(stderr, "error setting output file position to %u\n", (unsigned int) result[index].offset);
    		exit(-1);
		}
		if (fputc(result[index].data, file_out) == EOF) {
    		fprintf(stderr, "error writing byte %d to output file\n", result[index].data);
   			 exit(-1);
		}
		pthread_mutex_unlock(&mutexOUT);

		// critical section for writing to file 
		pthread_mutex_lock(&mutexWORK);
		result[index].data = '\0';
		result[index].state = 'e';
		result[index].offset = 0;
		pthread_mutex_unlock(&mutexWORK);
	}

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

	// create a buffer item and initialize the buffer
	result = (BufferItem*)malloc(sizeof(BufferItem));
	initialize_buffer();

	// error handling for user input
	int keyCheck = atoi(KEY);
	valid_key(keyCheck, "enter a valid integer as the key");
	valid_input(nIN, 1, "number of in threads should be at least 1");
	valid_input(nOUT, 1, "number of out threads should be at least 1");
	valid_input(nWORK, 1, "number of work threads should be at least 1");
	valid_input(bufSize, 1, "buffer size should be at least 1");
	valid_input(argc, 8, "follow this format: encrypt <KEY> <nIN> <nWORK> <nOUT> <file_in> <file_out> <bufSize>");
	
	// create as many in/work/out threads as user specified
	for (i = 1; i < nIN; i++){
		pthread_create(&INthreads[i], &attr, (void *) IN_thread, file_in);
	}
	for (i = 1; i < nWORK; i++){
		pthread_create(&WORKthreads[i], &attr, (void *) WORK_thread, KEY);
	}
	for (i = 1; i < nOUT; i++){
		pthread_create(&OUTthreads[i], &attr, (void *) OUT_thread, file_out);
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

	// close all files
	fclose(file_in);
	fclose(file_out);

	return 0;
}