# File transfer over TCP
A simple TCP connection between client to server, where the client uploads files from a
specific directory.

## Currently working on
Re-designing flow layout for handling pre-processed information in relation to memory
usage and computation usage.

## ToDo (Top-down Order)
    - Pull directory/file from server
    - Updating metadata of inode based on the client working directory to server directory
      (in working)
    - error checks on system calls and on failures (in working)
    - detecting TCP transmission failure and relay handling accordingly from client/server
      (in working)
    - Transferring symbolic links from client to server (in working)
    - Updating symbolic links based on file signature (designing)
    - Detect whether a file is specified and handling accordingly (designing)
    - Server response to current packages to client (designing)
    - Fix memory leaks in client/server (in working)
    - Fix data package for small/large files  (testing)
        - detect and resolve package lost (tesing)
    - Optimizing memory management through dynamic memory (in working)
    - Struct serialization (in working)
    - Encryption/decryption(*)
    - Client verification (designing)
    - Storing files/directories in client unique directory (testing)
    - TCP integrity(**)
    - Inode integrity(**)
    - Client/server log
        - file information 
        - client-side:
            - bytes read(IO)
            - bytes sent(TCP message)
            - client-side backlog(**)
        - server-side:
            - bytes read(TCP message)
            - bytes wrote(IO)
            - server-side backlog(**)
    - Verifications of all TCP [reviceve/send]
    - Check client/server arguments
        - type check
        - inode check
            - validity
            - permissions
    - Warning against overwriting existing inodes
    - Code style(**)

<pre>
Note
 (*): requires to be revised/worked
(**): stage design
</pre>

## Compilation and Execution
Intended to be compiled slightly different than usual, so a Makfile is needed (unless
command-line, alot of macros with slight modification to source and huge amount of free
time is a thing). No Makefile is uploaded to this repo due to various of reasons, but a
sample is shown below.

```
# Makefile Sample

# Secret hash
__SECRET__ = '"_____$$$SoMeRandOmSecRetHaSh$$$SoMeRandOmSecRetHaSh$$$SoMeRandOmSecRetHaSh$$$SoMeRandOmSecRetHaSh$$$SoMeRandOmSecRetHaSh$$$_____"'

# Macros
__UNSUPPORT_MESSAGE__ = '"\n\nThis DOES NOT and WILL NOT EVER support your whole storage at root...\n\n\nIf you actually require this you are clearly doing something wrong...\n\nWhy did I even allow this but... this is partically supportted.\nBut *WARNING* UNTESTED and NOT DESIGNED FOR......\n"'

__UNSUPPORT_MESSAGE_REASON__ = '"Reason: A generic copying of any file type \n[i.e inodes abstracted as drivers, object, \nfile and program architecture interpretation, etc]\ncreates unexpected errors during run-time due to how files are created for\nread/write, and including the specifics due to compiling programs\n\n"'

DEFAULT_KEY = 63   # Default key value
MASKLEN = 128      # Secret hash length
PORT = 4001        # Port number

# Compilation Flags
CFLAGS = -D_BSD_SOURCE -DDEFAULT_KEY=${DEFAULT_KEY} -DMASKLEN=${MASKLEN} -D__SECRET__=${__SECRET__} -D__UNSUPPORT_MESSAGE__=${__UNSUPPORT_MESSAGE__} -D__UNSUPPORT_MESSAGE_REASON__=${__UNSUPPORT_MESSAGE_REASON__} -DPORT=${PORT} -g -Wall -std=c99 -pedantic -I include/

DEPENDENCIES = include/*.h
EXE = server client

all: ${EXE}

server: bin/server.o bin/hash_functions.o bin/epackage_tcp.o bin/spackage_tcp.o bin/server_handler.o bin/mask.o
    gcc ${CFLAGS} -o $@ bin/server.o bin/hash_functions.o bin/epackage_tcp.o bin/spackage_tcp.o bin/server_handler.o bin/mask.o

client: bin/client.o bin/hash_functions.o bin/epackage_tcp.o bin/spackage_tcp.o bin/server_handler.o bin/mask.o
    gcc ${CFLAGS} -o $@ bin/client.o bin/hash_functions.o bin/epackage_tcp.o bin/spackage_tcp.o bin/server_handler.o bin/mask.o

bin/%.o: src/%.c ${DEPENDENCIES}
    gcc ${CFLAGS} -c $< -o $@

clean:
    rm bin/*.o #EXE   # can be left commented or removed depending on setup
```

Execution is as per-usual after compilation.

Read and lookup in source code for client and server execution instruction. A quick lookup
is given below, but the method of execution may change and it is prefered to lookup the
execution instruction from client and server source.

```
# Execution

# client side
./client 127.0.0.1 <target_local_directory> <target_remote_directory> <download|upload>

# server side
./server
```
