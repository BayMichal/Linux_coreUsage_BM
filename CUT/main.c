//----------------------------------------------------
//                  Michał Bajkos
//              Linux CPU Usage tracker 
//              Project Learning and reading time:  8h
//              Project Coding Time:                6h.
//----------------------------------------------------
    
#include <unistd.h>     /* Symbolic Constants       */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors           */
#include <stdio.h>      /* Input/Output         */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore use */
#include <time.h>       /* Time */
    
 
#define BUFOR_SIZE         1024
#define DATA_SIZE          100
#define OFFEST_DATA        100
#define STAT_FIELDS        10
#define ID_NBR_CORE        12
#define MAX_CORES          3


typedef struct {
    unsigned int  core_count;
    char cpu_core[BUFOR_SIZE];
    unsigned long long int Datebase[MAX_CORES][STAT_FIELDS];
    unsigned long long int prev_Datebase[MAX_CORES][STAT_FIELDS];
    double CPU_Percentage[MAX_CORES];
    
}dataStruct;

typedef struct{
    unsigned long long int Idle;
    unsigned long long int NonIdle;
    unsigned long long int Total;
}fieldStruct;

typedef struct{
    unsigned long long int previdle;
    unsigned long long int prevNonIdle;
    unsigned long long int prevTotal;
}prev_fieldStruct;

dataStruct sData;
fieldStruct sFieldData;
prev_fieldStruct prev_sFieldData;

sem_t IsEmpty;  //Bufor is empty, ready to read data.
sem_t IsFull;   //Bufor is full, ready to print.
sem_t IsReady;  //Bufor is print, ready to get empty.


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
    while(1){
    sem_wait(&IsEmpty);                      /* Task block */


      //  printf(" JESTEM W READER  \n ");
 
    
    
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
    
    //printf(" ZWALNIAM TASKA READER\n");
    sem_post(&IsFull);              /* TASK OPEN */
    
    }
    return NULL;
}



void *f_analiz(void *a)
{
    while(1){
    sem_wait(&IsFull);                      /* Task block */

    //puts(" JESTEM W ANALIZ\n ");
    int nbr_lopp_convert=0;                 
    int static id_Empty=0;
    int offset = 0;
 
    char static bufer[200];                 /* Bufer to concencrate data char array -> int long */

    for(int x=0; x<sData.core_count; x++)
    {
        offset=100*x;
        nbr_lopp_convert=0;
        id_Empty=0;
    
        while(1)
                {
                    if(nbr_lopp_convert == 10)  break;
                    for(int i=0; i<100; i++)
                    {
                        bufer[i] = sData.cpu_core[i+id_Empty+offset]; 
                        if(sData.cpu_core[i+id_Empty+offset] != ' ')   continue;  /* If data from buffor is empty -> new data */
                        else  id_Empty=id_Empty+i+1; break;
                        
                        
                    }

                    sData.Datebase[x][nbr_lopp_convert] = atoi(bufer); 
                    memset(bufer,0,200);
                    nbr_lopp_convert++;
                    
                }   
    }

    //----------------------Algorytm-------------------
unsigned long long int totald;
unsigned long long int idled;

for(int x = 0; x < sData.core_count; x++)
{

    prev_sFieldData.previdle = sData.prev_Datebase[x][3] + sData.prev_Datebase[x][4];
    sFieldData.Idle = sData.Datebase[x][3] + sData.Datebase[x][4];

    prev_sFieldData.prevNonIdle = sData.prev_Datebase[x][0] + sData.prev_Datebase[x][1] + sData.prev_Datebase[x][2] + sData.prev_Datebase[x][5] + sData.prev_Datebase[x][6] + sData.prev_Datebase[x][7];
    sFieldData.NonIdle = sData.Datebase[x][0] + sData.Datebase[x][1] + sData.Datebase[x][2] + sData.Datebase[x][5] + sData.Datebase[x][6] + sData.Datebase[x][7];

    prev_sFieldData.prevTotal = prev_sFieldData.previdle+ prev_sFieldData.prevNonIdle;
    sFieldData.Total = sFieldData.Idle + sFieldData.NonIdle;

    totald = sFieldData.Total - prev_sFieldData.prevTotal;
    idled = sFieldData.Idle - prev_sFieldData.previdle;

    sData.CPU_Percentage[x] = ((((double)totald - (double)idled) / (double)totald) );
}
    

    //printf(" ZWALNIAM TASKA ANALIZA\n");
    sem_post(&IsReady);
    }
    return NULL;

}

void *f_print(void *a)
{
    while(1){
    sem_wait(&IsReady);

   
    
        sleep(1);
      // puts(" JESTEM W PRINT\n ");
       puts("\n");
        for(int k=0; k<sData.core_count; k++)
        {
            printf(" Zuzycie rdzenia nr: %d wynosi: %f %% \n", k, sData.CPU_Percentage[k]);
        }
        
    

    /* SET 0 */
    memset(sData.prev_Datebase, 0 , sizeof(sData.prev_Datebase));

    /* SAVE DATA TO PREV DATA TO NEXT USE ALGORITHM */

    memcpy(sData.prev_Datebase, sData.Datebase, sizeof(sData.Datebase));
    memset(sData.cpu_core,0,sizeof(sData.cpu_core));

    
    sem_post(&IsEmpty);
    }
    return NULL;
}

void *f_watchdog(void *a)
{
    
   // puts(" JESTEM W WATCHDOG\n ");


    
}

void *f_logger(void *a)
{
    
   // puts(" JESTEM W LOGGER\n ");


}

int main(int argc, char *argv[])
{
    getCoreNumer(&sData);   /* Read how many cores system have */
    sem_init(&IsEmpty,0,1);
    sem_init(&IsFull,0,0);
    sem_init(&IsReady,0,0);

    pthread_t t_Reader,t_Analyzer,t_Printer,t_Watchdog,t_Logger,t_;
    


    if( pthread_create(&t_Reader, NULL, f_reader, NULL) == -1)      printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Analyzer, NULL, f_analiz, NULL) == -1)    printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Printer, NULL, f_print, NULL) == -1)      printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Watchdog, NULL, f_watchdog, NULL) == -1)  printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Logger, NULL, f_logger, NULL) == -1)      printf(" Nie mozna utworzyć wątku");


void *result;
    if(pthread_join(t_Reader, &result) == -1)   printf("Blad oczekiwania na zakonczenie watku tReader");
    if(pthread_join(t_Analyzer, &result) == -1) printf("Blad oczekiwania na zakonczenie watku t_Analyzer");
    if(pthread_join(t_Printer, &result) == -1)  printf("Blad oczekiwania na zakonczenie watku t_Printer");
    if(pthread_join(t_Watchdog, &result) == -1) printf("Blad oczekiwania na zakonczenie watku t_Watchdog");
    if(pthread_join(t_Logger, &result) == -1)   printf("Blad oczekiwania na zakonczenie watku t_Logger");


return 0;
}




