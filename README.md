# Operating Systems

5 Assignments that I submited for The Operating Systems class in Tel Aviv University

## Multi-Level Page Tables Assignment

An implementation of a simulated OS code that handles a multi-level (trie-based)
page table. There are two functions: The first function creates/destroys virtual memory
mappings in a page table. The second function checks if an address is mapped in a page table. (The
second function is needed if the OS wants to figure out which physical address a process virtual
address maps to.)

## Mini Shell Assignment

An implementation of a simple shell program: a function that receives a shell command and performs it. 

## Message Slot Kernel Module Assignment

An implementation of a kernel module that provides a new IPC mechanism, called a message slot. A message slot is a character device file through which processes communicate.
A message slot device has multiple message channels active concurrently, which can be used by
multiple processes. After opening a message slot device file, a process uses ioctl() to specify the
id of the message channel it wants to use. It subsequently uses read()/write() to receive/send
messages on the channel. In contrast to pipes, a message channel preserves a message until
it is overwritten, so the same message can be read multiple times.

## Parallel File Find Assignment

A program that searches a directory tree for files whose name matches
some search term. The program receives a directory D and a search term T, and finds every file
in D’s directory tree whose name contains T. The program parallelizes its work using threads.
Specifically, individual directories are searched by different threads.

## Printable Characters Counting Server Assignment

An implementation of a toy client/server architecture: a printable characters counting (PCC) server. Clients connect to the server and send it a stream of bytes. The server counts how many of the bytes are printable and returns that number to the client. The server also maintains overall statistics on the number of printable characters it has received from all clients. When the server terminates, it prints these statistics to standard output.
