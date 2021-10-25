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
     
 
#define _BUFOR_SIZE         1024
#define _OFFEST_ANSWER_CORE 100

typedef struct {
    int core_count;
    int cpu_core[_BUFOR_SIZE];

}struktura;


int main(int argc, char *argv[])
{
    struktura sData;
    struktura *poi_sDane;
    char data[_BUFOR_SIZE];                             /* Create data bufor */
    FILE *STAT = fopen("/proc/stat" ,"r");              /* Create File pointer with command to read file /proc/stat*/
    FILE *NMBR_CORES_N = fopen("/proc/cpuinfo" ,"r");   /* Create File pointer with command to read how many cores system have */
    
   




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
        for(int j=0; j<100; j++)
        {

                if(sData.cpu_core >= 0) sData.cpu_core[j + _OFFEST_ANSWER_CORE * i]  = (data[j+5] - '0');    /* Read data and convert to int */
                
    
        }
        i++;
    }


j=0;
    for(int z=0; z<sData.core_count; z++)       /* DEBUG PRINT */
    {
        printf("\n Core : ");
        for(int t=0; t<100; t++)
        {
            if(sData.cpu_core[t+j] >= 0)    printf("%d",sData.cpu_core[t+j]);
            else                            printf(" ");
        }
    j+=100;
          
    }
                                 
    return 0;
}
