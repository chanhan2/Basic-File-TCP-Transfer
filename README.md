# File transfer over TCP
    A simple TCP connection between client to server, where the client uploads files from a
    specific directory.

## ToDo (Top-down Order)
    - detecting TCP transmission failure and relay handling accordingly from client/server (in working)
    - updating symbolic links based on file signature (designing)
    - detect whether a file is specified and handling accordingly (designing)
    - server response to current packages to client (designing)
    - fix memory leaks in client/server (in working)
    - fix data package for small/large files  (testing)
        - detect and resolve package lost (tesing)
    - struct serialization (in working)
    - encryption/decryption(*)
    - client verification (designing)
    - storing files/directories in client unique directory (testing)
    - code style(**)

<pre>
Note
 (*): requires to be revised/worked
(**): stage design
</pre>