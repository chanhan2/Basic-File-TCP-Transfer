# File transfer over TCP
    A simple TCP connection between client to server, where the client uploads files from a
    specific directory.

## ToDo (Top-down Order)
    - Updating metadata of inode based on the client working directory to server directory (in working)
    - error checks on system calls and on failures (in working)
    - detecting TCP transmission failure and relay handling accordingly from client/server (in working)
    - transferring symbolic links from client to server (in working)
    - updating symbolic links based on file signature (designing)
    - detect whether a file is specified and handling accordingly (designing)
    - server response to current packages to client (designing)
    - fix memory leaks in client/server (in working)
    - fix data package for small/large files  (testing)
        - detect and resolve package lost (tesing)
    - optimizing memory management through dynamic memory (in working)
    - struct serialization (in working)
    - encryption/decryption(*)
    - client verification (designing)
    - storing files/directories in client unique directory (testing)
    - TCP integrity(**)
    - inode integrity(**)
    - client/server log
        - file information 
        - client-side:
            - bytes read(IO)
            - bytes sent(TCP message)
            - client-side backlog(**)
        - server-side:
            - bytes read(TCP message)
            - bytes wrote(IO)
            - server-side backlog(**)
    - verifications of all TCP [reviceve/send]
    - check client/server arguments
        - type check
        - inode check
            - validity
            - permissions
    - code style(**)

<pre>
Note
 (*): requires to be revised/worked
(**): stage design
</pre>