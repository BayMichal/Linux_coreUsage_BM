//----------------------------------------------------
//                  Michał Bajkos
//              Linux CPU Usage tracker 
//              Project Learning and reading time:  8h
//              Project Coding Time:                6h.
//----------------------------------------------------
    
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */

    
 
#define _BUFOR_SIZE         1024
#define _OFFEST_ANSWER_CORE 100

typedef struct {
    int  core_count;
    char cpu_core[_BUFOR_SIZE];
    char value[10];

}struktura;


void *Reader(void *a){}

void *Analyzer(void *a){}

void *Printer(void *a){}

void *Watchdog(void *a){}

void *Logger(void *a){}



int main(int argc, char *argv[])
{
    struktura sData;
    struktura *poi_sDane;
    char data[_BUFOR_SIZE];                             /* Create data bufor */
    FILE *STAT = fopen("/proc/stat" ,"r");              /* Create File pointer with command to read file /proc/stat*/
    FILE *NMBR_CORES_N = fopen("/proc/cpuinfo" ,"r");   /* Create File pointer with command to read how many cores system have */
   
    
    long int try[20];
    char quick_buf[200];


int j=0;
    while( fgets(data,1024,NMBR_CORES_N)  != NULL )             
    {
        if(j == 11)
        {
            sData.core_count = ( data[12] - '0');       /* Convert char to int */
            if(!sData.core_count)    puts("Error core count");
            break;
        }
        j++;
    }


int i=0;
    while( fgets(data,1024,STAT) != NULL)             
    {
        if(i <= sData.core_count ) 
        {
            for(int j=0; j<100; j++)
            {   
               sData.cpu_core[j + _OFFEST_ANSWER_CORE * i]  = data[j+5];    /* Read data and convert to int */
            }
        }
        else    break; 
        i++;
    }


int static nbr_to_space, nbr_loop;
char static bufer[200];
while(1)
{
    if(nbr_loop == 20)  break;
    for(i=0; i<100; i++)
    {
        bufer[i] = sData.cpu_core[i+nbr_to_space];
        if(sData.cpu_core[i] != ' ')  continue;
        else                nbr_to_space=nbr_to_space+i; break;
    }
    strcat(quick_buf+nbr_to_space, bufer);
    try[nbr_loop] = atoi(bufer); 
    memset(bufer,0,200);
    nbr_loop++;
}

printf( " %ld \n", try[0]);
printf( " %ld \n", try[1]);
printf( " %ld \n", try[2]);
    return 0;
}



