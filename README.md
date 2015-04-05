# Encyption-With-Threads
C program that creates three groups of Pthreads to encrypt an input text file into a secret code or decrypt the secret code into the original text file according to a given KEY value.

# Build
Compile with
```
gcc -Wall -o encrypt encrypt.c -lpthread
```

# Usage
Run with
```
 encrypt <KEY> <nIN> <nWORK> <nOUT> <file_in> <file_out> <bufSize>
```

# License
MIT © Ursula Sarracini
