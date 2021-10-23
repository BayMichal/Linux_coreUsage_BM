//----------------------------------------------------
//                  Michał Bajkos
//              Linux CPU Usage tracker 
//              Project Learning and reading time:  4h
//              Project Coding Time:                30min.
//----------------------------------------------------
    
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
     
 
#define _BUFOR_SIZE      1024

int main(int argc, char *argv[])
{
    
    char data[_BUFOR_SIZE];                        /* Create data bufor */
    FILE *fd = fopen("/proc/stat" ,"r");           /* Create File pointer with command to read file /proc/stat*/
    fgets(data,_BUFOR_SIZE,fd);           /* Write Data to buffer */
    puts(data);                                    /* Put data into terminal */


    return 0;
}
