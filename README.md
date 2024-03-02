# breakpoint_kernel_module
Linux kernel module for creating breakpoints with read/write handlers for specified address
Generally developed for i686 architecture and tested using qemux86 with builed Yocto/poky image

# Building
For building you can just use `make`.  

# Testing
For testing i created simple C program which allocates 1 byte of memory, prints address of it, waits 10 seconds, writes some data to allocated address, waits 10 seconds, reads data from allocated data, sleeps 10 secs and freeying memory.  
Once address printed you should enable module using:  
```bash
insmod memory.ko watch_address=0x48b1a0
```
And just see the backtraces of write and read breakpoints.  
test.C code:
```C
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    unsigned char *ptr = malloc(1);
    printf("Address of ptr: %p\n", ptr);
    printf("Waiting 10 secs...\n");
    sleep(10);
    printf("Writing!\n");
    *ptr = 0xAB;
    printf("Sleeping 10 secs\n");
    sleep(10);
    printf("Reading!\n");
    printf("Data at the memory address: %d\n", *ptr);
    printf("Sleeping 10 secs\n");
    sleep(10);
    printf("Free\n");
 

    return 0;
}
```
You can compile it using gcc and run in background:  
```bash
gcc test.c -o test
./test &
```
