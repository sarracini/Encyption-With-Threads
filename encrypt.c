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
 -ALL the error handling
 -test test test
*/
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
	//int empty;
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
	int index;
	char curr;
	off_t offset;

	thread_sleep();
	
	do{

		pthread_mutex_lock(&mutexWORK);
		index = first_empty_item_in_buffer();			
		
		while (index > -1){
			
			// critical section to read in file 
			pthread_mutex_lock(&mutexIN);
			offset = ftell(file_in);
			curr = fgetc(file_in);
			pthread_mutex_unlock(&mutexIN);

			if (curr == EOF){
				break;
			}
			else{ 
				result[index].offset = offset;
				result[index].data = curr;
				result[index].state = 'w';
				index = first_empty_item_in_buffer();
			}

		}
		pthread_mutex_unlock(&mutexWORK);
		thread_sleep();	
	}while (!feof(file_in));

	pthread_mutex_lock(&mutexWORK);
	active_in--;
	pthread_mutex_unlock(&mutexWORK);
	
	return NULL;
}

void *WORK_thread(void *param){
	int index = 0;
	int local_active_in;
	char curr;
	int key = atoi(param);
	
	thread_sleep();

	do{

		pthread_mutex_lock(&mutexWORK);
		index = first_work_item_in_buffer();
		//while(is_buffer_empty()) {
		//	thread_sleep();
		//}
		if (index > -1){
			
			curr = result[index].data;

			if (curr == EOF){
				break;
			}
			// encrypting/decrypting file if there is work to be done and buffer is non-empty
			if (key >= 0 && curr > 31 && curr < 127){
				curr = (((int)curr-32)+2*95+key)%95+32;
			}
			else if (key < 0 && curr > 31 && curr < 127){
				curr = (((int)curr-32)+2*95-(-1*key))%95+32;
			}
			// critical section to write encrypted character back to buffer, change state and grab next work byte
			result[index].data = curr;
			result[index].state = 'o';
		}
		
		local_active_in = active_in;
		pthread_mutex_unlock(&mutexWORK);

	}while (index > -1 || local_active_in > 0);

	pthread_mutex_lock(&mutexWORK);
	active_work--;
	pthread_mutex_unlock(&mutexWORK);

	return NULL;
}

void *OUT_thread(void *param){
	int index = 0;
	char curr;
	off_t offset;
	int local_active_work;

	thread_sleep();

		
	do{
		pthread_mutex_lock(&mutexWORK);
		index = first_out_item_in_buffer();
		
		if (index > -1){
			offset = result[index].offset;
			curr = result[index].data;

			// critical section for writing to file 
			pthread_mutex_lock(&mutexOUT);
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
			result[index].data = '\0';
			result[index].state = 'e';
			result[index].offset = 0;
		}
		local_active_work = active_work;
		pthread_mutex_unlock(&mutexWORK);
		thread_sleep();
	}while (index > -1 || local_active_work > 0);
	
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

	// close all files
	fclose(file_in);
	fclose(file_out);
	free(result);

	return 0;
}