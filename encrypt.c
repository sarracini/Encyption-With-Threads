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
int active_in;
int active_work;
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

// putting the thread to sleep for a random amount of time
void thread_sleep(void){
	struct timespec t;
	int seed = 0;
	t.tv_sec = 0;
	t.tv_nsec = rand_r((unsigned int*)&seed)%(TEN_MILLIS_IN_NANOS+1);
	nanosleep(&t, NULL);
}

// check if the buffer is empty
int is_buffer_empty(){
	int i = 0;
	while (i < bufSize){
		if (result[i].state == 'e'){
			return 1;
		}
		i++;
	}
	return 0;
}

// return the first empty item in the buffer
int first_empty_item_in_buffer(){
	int i = 0;
	if (is_buffer_empty()){
		while (i < bufSize){
			if (result[i].state == 'e'){
				return i;
			}
			i++;
		}
	}
	return -1;
}

// return the first item that is ready to be worked on in the buffer
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

// return the first encrypted item to be written to the file from the buffer
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

// initialize the buffer - set all states to empty
void initialize_buffer(){
	int i = 0;
	while (i < bufSize){
		result[i].state = 'e';
		i++;
	}
}

// error handling for valid user input
void valid_input(int param, int expected, char* msg){
	if (param < expected){
		fprintf(stderr, "%s\n", msg);
		exit(-1);
	}
}

// error handling for valid user key
void valid_key(int param, char* msg){
	if (param > 127 || param < -127){
		fprintf(stderr, "%s\n", msg);
	}
}

// the in thread - read from file and write to buffer
void *IN_thread(void *param){
	int index;
	char curr;
	off_t offset;

	thread_sleep();
	
	do {
		// critical section for returning first empty item in buffer
		pthread_mutex_lock(&mutexWORK);
		index = first_empty_item_in_buffer();			

		while (index > -1){

			// if the buffer is empty - sleep
			if (is_buffer_empty() == 1) {
				thread_sleep();
			}
			
			// critical section to read in file and store character
			pthread_mutex_lock(&mutexIN);
			offset = ftell(file_in);
			curr = fgetc(file_in);
			pthread_mutex_unlock(&mutexIN);

			if (curr == EOF){
				break;
			}

			else{ 
				// store character to buffer and indicate that it is ready for work then grab next item
				result[index].offset = offset;
				result[index].data = curr;
				result[index].state = 'w';
				index = first_empty_item_in_buffer();
			}
		}

		pthread_mutex_unlock(&mutexWORK);

	} while (!feof(file_in));

	thread_sleep();	

	// decrease the number of active in threads 
	pthread_mutex_lock(&mutexWORK);
	active_in--;
	pthread_mutex_unlock(&mutexWORK);
	
	return NULL;
}

// the work thread - read from the buffer, encrypt the character and write back to buffer
void *WORK_thread(void *param){
	int index = 0;
	int local_active_in;
	char curr;
	int key = atoi(param);
	
	thread_sleep();

	do {
		// critical section tp read in the first item ready to be encrypted
		pthread_mutex_lock(&mutexWORK);
		index = first_work_item_in_buffer();
		
		if (index > -1){
			
			// store that current item in the buffer
			curr = result[index].data;
			
			// if the buffer is empty - sleep
			if (is_buffer_empty() == 1) {
				thread_sleep();
			}
			
			if (curr == EOF || curr == '\0'){
				break;
			}
			
			// encrypting/derypting the single current character
			if (key >= 0 && curr > 31 && curr < 127){
				curr = (((int)curr-32)+2*95+key)%95+32;
			}
			else if (key < 0 && curr > 31 && curr < 127){
				curr = (((int)curr-32)+2*95-(-1*key))%95+32;
			}

			// write encrypted character back to buffer, indicate it's ready to be outputted
			result[index].data = curr;
			result[index].state = 'o';
		}
		
		local_active_in = active_in;
		pthread_mutex_unlock(&mutexWORK);

	} while (index > -1 || local_active_in > 0);

	thread_sleep();	

	// decrease the number of active work threads
	pthread_mutex_lock(&mutexWORK);
	active_work--;
	pthread_mutex_unlock(&mutexWORK);

	return NULL;
}

// the output thread - read from buffer and write to file
void *OUT_thread(void *param){
	int index = 0;
	char curr;
	off_t offset;
	int local_active_work;

	thread_sleep();

	do {

		// critical section to read in the first item to be outputted
		pthread_mutex_lock(&mutexWORK);
		index = first_out_item_in_buffer();

		if (index > -1){

			// store that current character 
			offset = result[index].offset;
			curr = result[index].data;

			// if the buffer is empty - sleep
			if (is_buffer_empty() == 1) {
				thread_sleep();
			}

			// critical section for writing to file 
			pthread_mutex_lock(&mutexOUT);
			if (fseek(file_out, offset, SEEK_SET) == -1) {
    			fprintf(stderr, "error setting output file position to %u\n", (unsigned int) offset);
    			exit(-1);
			}
			if (fputc(curr, file_out) == EOF) {
    			fprintf(stderr, "error writing byte %d to output file\n", curr);
   				 exit(-1);
			}
			pthread_mutex_unlock(&mutexOUT);

			// store empty character to buffer and indicate that it is empty
			result[index].data = '\0';
			result[index].state = 'e';
			result[index].offset = 0;
		}

		local_active_work = active_work;
		pthread_mutex_unlock(&mutexWORK);
	
	} while (index > -1 || local_active_work > 0);
	
	thread_sleep();
	
	return NULL;
}

int main(int argc, char *argv[]){
	int i = 0;
	int nIN;
	int nOUT;
	int nWORK; 
	
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
	active_in = nIN;
	active_work = nWORK;

	// threads
	pthread_t INthreads[nIN];
	pthread_t OUTthreads[nOUT];
	pthread_t WORKthreads[nWORK];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	// create a buffer item and initialize the buffer
	result = (BufferItem*)malloc(sizeof(BufferItem)*bufSize);
	initialize_buffer();

	// error handling for user input
	int keyCheck = atoi(KEY);
	valid_key(keyCheck, "enter a valid integer as the key");
	valid_input(nIN, 1, "number of in threads should be at least 1");
	valid_input(nOUT, 1, "number of out threads should be at least 1");
	valid_input(nWORK, 1, "number of work threads should be at least 1");
	valid_input(bufSize, 1, "buffer size should be at least 1");
	valid_input(argc, 8, "follow this format: encrypt <KEY> <nIN> <nWORK> <nOUT> <file_in> <file_out> <bufSize>");
	if (file_in == NULL){
		fprintf(stderr, "could not open input file for reading\n");
	}
	if (file_out == NULL){
		fprintf(stderr, "could not open input file for writing\n");
	}
	
	// create as many in/work/out threads as user specified
	for (i = 0; i < nIN; i++){
		pthread_create(&INthreads[i], &attr, (void *) IN_thread, file_in);
	}
	for (i = 0; i < nWORK; i++){
		pthread_create(&WORKthreads[i], &attr, (void *) WORK_thread, KEY);
	}
	for (i = 0; i < nOUT; i++){
		pthread_create(&OUTthreads[i], &attr, (void *) OUT_thread, file_out);
	}

	// join all the threads
	for (i = 0; i < nIN; i++){
		pthread_join(INthreads[i], NULL);
	}
	for (i = 0; i < nWORK; i++){
		pthread_join(WORKthreads[i], NULL);
	}
	for (i = 0; i < nOUT; i++){
		pthread_join(OUTthreads[i], NULL);
	}

	// destory all mutexes
	pthread_mutex_destroy(&mutexIN);
	pthread_mutex_destroy(&mutexOUT);
	pthread_mutex_destroy(&mutexWORK);

	// close all files and free buffer
	fclose(file_in);
	fclose(file_out);
	free(result);

	return 0;
}