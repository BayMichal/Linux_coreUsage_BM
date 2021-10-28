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
#include <semaphore.h>
    
 
#define BUFOR_SIZE         1024
#define DATA_SIZE          100
#define OFFEST_DATA        100
#define STAT_FIELDS        10
#define ID_NBR_CORE        12




typedef struct {
    int  core_count;
    char cpu_core[BUFOR_SIZE];
    long int Datebase[10][STAT_FIELDS];

}dataStruct;

dataStruct sData;
dataStruct *pData;
sem_t ticket;


char data[BUFOR_SIZE];                             /* Create data char bufor */




void getCoreNumer(dataStruct  *context)
{
    puts(" JESTEM W ODCZYTYWANIU CORE \n ");
    FILE *NMBR_CORES_N = fopen("/proc/cpuinfo" ,"r");   /* Create File pointer with command to read how many cores system have */

    int j=0;
    while( fgets(data,BUFOR_SIZE,NMBR_CORES_N)  != NULL )             
    {
        if(j == 11)
        {
            context->core_count = ( data[ID_NBR_CORE] - '0');       /* Convert char to int */
            if(!context->core_count)    puts("Error core count");
            break;
        }
        j++;
    }
}






void *f_reader(void *a)
{
    sem_wait(&ticket);                      /* Task block */


        printf(" JESTEM W READER  \n ");
 
    
    
    int rows_data=0;                        /* variable describe number of row data from terminal */
    int nbr_lopp_convert=0; 
    int static id_Empty=0;
    int i=0;
    

    FILE *STAT = fopen("/proc/stat" ,"r");  /* Create File pointer with command to read file /proc/stat*/
    while( fgets(data,BUFOR_SIZE,STAT) != NULL)             
    {
        if(rows_data < sData.core_count ) /* if number rows of data == number of cores */  
        {
            nbr_lopp_convert=0;
            id_Empty=0;
            for(int j=0; j<DATA_SIZE; j++)
            {   
               sData.cpu_core[j+ OFFEST_DATA * i]  = data[j+5];    /* Read data and convert to int */
            }

        }
        else    break; 
        i++;
        rows_data++;
    }
    
    printf(" ZWALNIAM TASKA READER\n");
    sem_post(&ticket);              /* TASK OPEN */
    return NULL;
}



void *f_analiz(void *a)
{
    sem_wait(&ticket);                      /* Task block */

    puts(" JESTEM W ANALIZ\n ");
    int nbr_lopp_convert=0;                 
    int static id_Empty=0;
    int jado = 0;
 
    char static bufer[200];                 /* Bufer to concencrate data char array -> int long */

    for(int x=0; x<sData.core_count; x++)
    {
        jado=100*x;
        nbr_lopp_convert=0;
        id_Empty=0;
    
        while(1)
                {
                    if(nbr_lopp_convert == 10)  break;
                    for(int i=0; i<100; i++)
                    {
                        bufer[i] = sData.cpu_core[i+id_Empty+jado]; 
                        if(sData.cpu_core[i+id_Empty] != ' ')   continue;  /* If data from buffor is empty -> new data */
                        else  id_Empty=id_Empty+i+1; break;
                        
                        
                    }

                    sData.Datebase[x][nbr_lopp_convert] = atoi(bufer); 
                    memset(bufer,0,200);
                    nbr_lopp_convert++;
                    
                }   
    }
    printf(" ZWALNIAM TASKA ANALIZA\n");
    sem_post(&ticket);

    return NULL;

}

void *f_print(void *a)
{
    sem_wait(&ticket);                      /* Task block */

    puts(" JESTEM W PRINT\n ");
    for(int k=0; k<sData.core_count; k++)
    {
        printf(" core nbr: %d \n", k);
        for(int j=0; j<10; j++)
        {

        printf(" %ld \n", sData.Datebase[k][j]);

        }
        printf("\n ");
    }
    printf(" ZWALNIAM TASKA PRINTF\n");
    sem_post(&ticket);

    return NULL;
}

void *f_watchdog(void *a)
{
    sem_wait(&ticket);                      /* Task block */
    puts(" JESTEM W WATCHDOG\n ");

    puts(" ZWALNIAM TASKA WATCHDOG\n");
    sem_post(&ticket);
}

void *f_logger(void *a)
{
    sem_wait(&ticket);                      /* Task block */
    puts(" JESTEM W LOGGER\n ");

    puts(" ZWALNIAM TASKA LOGGER\n");
    sem_post(&ticket);
}

int main(int argc, char *argv[])
{
    getCoreNumer(&sData);   /* Read how many cores system have */

    pthread_t t_Reader,t_Analyzer,t_Printer,t_Watchdog,t_Logger,t_;
    sem_init(&ticket,0,1);


    if( pthread_create(&t_Reader, NULL, f_reader, NULL) == -1)      printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Analyzer, NULL, f_analiz, NULL) == -1)    printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Printer, NULL, f_print, NULL) == -1)      printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Watchdog, NULL, f_watchdog, NULL) == -1)  printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Logger, NULL, f_logger, NULL) == -1)      printf(" Nie mozna utworzyć wątku");


void *result;
    if(pthread_join(t_Reader, &result) == -1)   printf("Blad oczekiwania na zakonczenie watku tReader");
    if(pthread_join(t_Analyzer, &result) == -1) printf("Blad oczekiwania na zakonczenie watku tReader");
    if(pthread_join(t_Printer, &result) == -1)  printf("Blad oczekiwania na zakonczenie watku tReader");
    if(pthread_join(t_Watchdog, &result) == -1) printf("Blad oczekiwania na zakonczenie watku tReader");
    if(pthread_join(t_Logger, &result) == -1)   printf("Blad oczekiwania na zakonczenie watku tReader");


return 0;
}




