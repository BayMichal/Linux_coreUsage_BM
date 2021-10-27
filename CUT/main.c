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

    
 
#define BUFOR_SIZE         1024
#define OFFEST_DATA        100
#define STAT_FIELDS        10



typedef struct {
    int  core_count;
    char cpu_core[BUFOR_SIZE];


}dataStruct;

dataStruct sData;
dataStruct *pData;


char data[BUFOR_SIZE];                             /* Create data char bufor */

void *Reader(void *a){}

void *Analyzer(void *a){}

void *Printer(void *a){}

void *Watchdog(void *a){}

void *Logger(void *a){}


void getCoreNumer(dataStruct  *context)
{
    FILE *NMBR_CORES_N = fopen("/proc/cpuinfo" ,"r");   /* Create File pointer with command to read how many cores system have */

    
    int j=0;
    while( fgets(data,1024,NMBR_CORES_N)  != NULL )             
    {
        if(j == 11)
        {
            context->core_count = ( data[12] - '0');       /* Convert char to int */
            if(!context->core_count)    puts("Error core count");
            break;
        }
        j++;
    }
}



int main(int argc, char *argv[])
{
    
getCoreNumer(&sData);   /* Read how many cores system have */


FILE *STAT = fopen("/proc/stat" ,"r");  /* Create File pointer with command to read file /proc/stat*/
int nbr_lopp_convert=0;                 /* Variable responsible for read 10 number of rows_data data from terminal */
int static id_Empty=0;
char static bufer[200];                 /* Bufer to concencrate data char array -> int long */
int rows_data=0;                        /* variable describe number of row data from terminal */




 long int Datebase[sData.core_count][STAT_FIELDS];

    while( fgets(data,1024,STAT) != NULL)             
    {
        if(rows_data < sData.core_count ) 
        {
            nbr_lopp_convert=0;
            id_Empty=0;
            for(int j=0; j<100; j++)
            {   
               sData.cpu_core[j]  = data[j+5];    /* Read data and convert to int */
            }
            while(1)
            {
                if(nbr_lopp_convert == 10)  break;
                for(int i=0; i<BUFOR_SIZE; i++)
                {
                    bufer[i] = sData.cpu_core[i+id_Empty]; 
                    if(sData.cpu_core[i+id_Empty] != ' ')   continue;   /* If data from buffor is empty -> new data */
                    else  id_Empty=id_Empty+i+1; break;
                    
                }

                Datebase[rows_data][nbr_lopp_convert] = atoi(bufer); 
                memset(bufer,0,200);
                nbr_lopp_convert++;
            }
        }
        else    break; 
        rows_data++;
    }

/* DEBUG PRINT */
for(int k=0; k<sData.core_count; k++)
{
    printf(" core nbr: %d \n", k);
    for(int j=0; j<10; j++)
    {

        printf(" %ld \n", Datebase[k][j]);

    }
    printf("\n ");
}
    return 0;
}



