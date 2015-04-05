# Encyption-With-Threads
C program that creates three groups of Pthreads to encrypt an input text file into a secret code or decrypt the secret code into the original text file according to a given KEY value.

Each IN thread goes to sleep (use nanosleep) for some random time between 0 and 0.01 seconds upon being created. Then, it reads the next single byte from the input file and saves that byte and its offset in the file to the next available empty slot in the buffer. Then, this IN threads goes to sleep (use nanosleep) for some random time between 0 and 0.01 seconds and then goes back to read the next byte of the file until the end of file. If the buffer is full, IN threads go to sleep (use nanosleep) for some random time between 0 and 0.01 seconds and then go back to check again.
 
Meanwhile, upon being created each WORK thread sleeps (use nanosleep) for some random time between 0 and 0.01 seconds and it reads next byte in the buffer and process one byte of data, either encrypts or decrypt according to the working mode. Then the WORK thread goes to sleep (use nanosleep) for some random time between 0 and 0.01 seconds and goes back to process next byte in the buffer until the entire file is done. If the buffer is empty, the WORK threads go to sleep (use nanosleep) for some random time between 0 and 0.01 seconds and then go back to check again.  If running in the encrypt mode, each WORK thread will encrypt each data byte in the buffer, from original ASCII code to secret code for each character in the file, according to the following formula:
 ```
   if (data>31 && data<127 )
         data = (((int)data-32)+2*95+KEY)%95+32 ;
 ```
If running in the decrypt mode, each WORK thread decrypts each data in the buffer, from secret code to original ASCII code, according to the following formula:
``` 
    if (data>31 && data<127 )
        data = (((int)data-32)+2*95-KEY)%95+32 ;
 ```
where KEY is a key value (between 0 and 127). If you use the same value for both encryption and decryption, it can perfectly recover the original data.
 
Similarly, upon being created, each OUT thread sleeps (use nanosleep) for some random time between 0 and 0.01 seconds and it reads a processed byte and its offset from the next available nonempty buffer slot, and then writes the byte to that offset in the target file. Then, it also goes to sleep (use nanosleep) for some random time between 0 and 0.01 seconds and goes back to copy next byte until nothing is left. If the buffer is empty, the OUT threads go to sleep (use nanosleep) for some random time between 0 and 0.01 seconds and then go back to check again.

# Build
Compile with ```$ make``` or alternatively compile manually with
```
$ gcc -Wall -o encrypt encrypt.c -lpthread
```

# Usage
Run with
```
$ encrypt <KEY> <nIN> <nWORK> <nOUT> <file_in> <file_out> <bufSize>
```

<b>KEY</b>: the key value used for encryption or decryption, and its valid value is from -127 to 127. If it is positive, WORK threads use  <KEY> as the KEY value to encrypt each data byte. If it is negative, WORK threads use the absolute value of <KEY> as the key value to decrypt each data byte.

<b>nIN</b>: the number of IN threads to create. There should be at least 1.

<b>nWORK</b>: the number of WORK threads to create. There should be at least 1.

<b>nOUT</b>:  the number of OUT threads to create. There should be at least 1.

<b>file_in</b>: the pathname of the file to be converted. It should exist and be readable.

<b>file_out</b>: the name to be given to the target file. If a file with that name already exists, it should be overwritten.

<b>bufSize</b>:  the capacity, in terms of BufferItem’s, of the  shared buffer. This should be at least 1.

# Notes
A text file ```test.txt``` is given to run with to test the encryption. If you run encrypt on test.txt twice with the same key, the file should be perfectly recovered. 
PS no deadlock! Woohoo!

# License
MIT © Ursula Sarracini
