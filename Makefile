CC=gcc
FILE_NAME=encrypt
all:	
		@$(CC) -Wall -o $(FILE_NAME) $(FILE_NAME).c  -lpthread