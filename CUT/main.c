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

#include <fcntl.h>      /* WR,RD to WDT */
#include <sys/ioctl.h>  /* Watchdog */
#include <linux/watchdog.h>/* Typedef for wdt */
#include <signal.h>     /* Sigterm */
    
 
#define BUFOR_SIZE         1024
#define DATA_SIZE          100
#define OFFEST_DATA        100
#define ID_NBR_CORE        12
#define MAX_CORES          10

#define F_USER              0   /* User Field   */
#define F_NICE              1   /* Nice Field   */
#define F_SYST              2   /* System Field */
#define F_IDLE              3   /* Idle Field   */
#define F_IOWR              4   /* IOWait Field */
#define F_IRQ               5   /* IRQ Field    */
#define F_SIRQ              6   /* Soft Irq     */
#define F_STEAL             7   /* Steal Field  */
#define F_QUEST             8   /* Quest        */
#define F_Q_NICE            9   /* Quest Nice   */
#define STAT_FIELDS         (F_USER + F_Q_NICE) /* 10 Stat Fields */
#define WATCHDOG_TIMEOUT    120


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

typedef struct{
    unsigned char wdt_Reader;
    unsigned char wdt_Analyzer;
    unsigned char wdt_Printer;
    unsigned char wdt_Logger;
}wdt_data;

dataStruct sData;
fieldStruct sFieldData;
prev_fieldStruct prev_sFieldData;
wdt_data wdt_sFlags;

sem_t IsEmpty;  //Bufor is empty, ready to read data.
sem_t IsFull;   //Bufor is full, ready to print.
sem_t IsReady;  //Bufor is print, ready to get empty.

char data[BUFOR_SIZE];                             /* Create data char bufor */

volatile sig_atomic_t done = 0;

void term(int signum){
    done = 1;
}


void getCoreNumer(dataStruct  *context)
{
    //puts(" JESTEM W ODCZYTYWANIU CORE \n ");
    FILE *NMBR_CORES_N = fopen("/proc/cpuinfo" ,"r");   /* Create File pointer with command to read how many cores system have */
    int j = 0;
    while( fgets(data, BUFOR_SIZE, NMBR_CORES_N)  != NULL )             
    {
        if (j == 11){
            context->core_count = ( data[ID_NBR_CORE] - '0');       /* Convert char to int */
            if(!context->core_count){
                puts("Error core count");
            }    
            break;
        }
        j++;
    }
}


void *f_reader(void *a)
{
    while(!done){
    sem_wait(&IsEmpty);                      /* Task block */
        //  printf(" JESTEM W READER  \n ");    
        int rows_data = 0;                        /* variable describe number of row data from terminal */
        int nbr_lopp_convert = 0; 
        int static id_Empty = 0;
        int i = 0;
        
        FILE *STAT = fopen("/proc/stat", "r");  /* Create File pointer with command to read file /proc/stat*/
        while( fgets(data, BUFOR_SIZE, STAT) != NULL)             
        {
            if(rows_data < sData.core_count ) 
            {
                nbr_lopp_convert = 0;
                id_Empty = 0;
                for(int j = 0; j < DATA_SIZE; j++)
                {   
                    sData.cpu_core[j+ OFFEST_DATA * i]  = data[j+5];    /* Read data and convert to int */
                }
            }
            else{
                break;
            } 
            i++;
            rows_data++;
        }
        wdt_sFlags.wdt_Reader = 1;
    sem_post(&IsFull);              /* TASK OPEN */
    }
    return NULL;
}

void *f_analiz(void *a)
{
    while(!done){
    sem_wait(&IsFull);                      /* Task block */
        //puts(" JESTEM W ANALIZ\n ");
        int nbr_lopp_convert = 0;                 
        int static id_Empty = 0;
        int offset = 0;
        char static bufer[200];                 /* Bufer to concencrate data char array -> int long */

        for(int x = 0; x < sData.core_count; x++)
        {
            offset = 100*x;
            nbr_lopp_convert = 0;
            id_Empty = 0;
        
            while(1)
            {
                if(nbr_lopp_convert == 10)  {
                    break;
                }
                for(int i = 0; i < 100; i++)
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

        for(int cnt_core = 0; cnt_core < sData.core_count; cnt_core++)
        {
            prev_sFieldData.previdle = sData.prev_Datebase[cnt_core][F_IDLE] + sData.prev_Datebase[cnt_core][F_IOWR];
            sFieldData.Idle = sData.Datebase[cnt_core][F_IDLE] + sData.Datebase[cnt_core][F_IOWR];

            prev_sFieldData.prevNonIdle = sData.prev_Datebase[cnt_core][F_USER] + sData.prev_Datebase[cnt_core][F_NICE] + sData.prev_Datebase[cnt_core][F_SYST] + sData.prev_Datebase[cnt_core][F_IRQ] + sData.prev_Datebase[cnt_core][F_SIRQ] + sData.prev_Datebase[cnt_core][F_STEAL];
            sFieldData.NonIdle = sData.Datebase[cnt_core][F_USER] + sData.Datebase[cnt_core][F_NICE] + sData.Datebase[cnt_core][F_SYST] + sData.Datebase[cnt_core][F_IRQ] + sData.Datebase[cnt_core][F_SIRQ] + sData.Datebase[cnt_core][F_STEAL];
            
            prev_sFieldData.prevTotal = prev_sFieldData.previdle+ prev_sFieldData.prevNonIdle;
            sFieldData.Total = sFieldData.Idle + sFieldData.NonIdle;
            
            totald = sFieldData.Total - prev_sFieldData.prevTotal;
            idled = sFieldData.Idle - prev_sFieldData.previdle;

            sData.CPU_Percentage[cnt_core] = ((((double)totald - (double)idled) / (double)totald) * 100);
        }
        //printf(" ZWALNIAM TASKA ANALIZA\n");
    wdt_sFlags.wdt_Analyzer = 1;
    sem_post(&IsReady);
    }
    return NULL;

}

void *f_print(void *a)
{
    while(!done){
    sem_wait(&IsReady);
        sleep(1);
        // puts(" JESTEM W PRINT\n ");
        puts("\n");
        for(int k = 0; k < sData.core_count; k++)
        {
            printf(" Zuzycie rdzenia nr: %d wynosi: %f %% \n", k, sData.CPU_Percentage[k]);
        }

        /* SET 0 */
        memset(sData.prev_Datebase, 0 , sizeof(sData.prev_Datebase));

        /* SAVE DATA TO PREV DATA TO NEXT USE ALGORITHM */
        memcpy(sData.prev_Datebase, sData.Datebase, sizeof(sData.Datebase));
        memset(sData.cpu_core, 0, sizeof(sData.cpu_core));
    wdt_sFlags.wdt_Printer = 1;
    sem_post(&IsEmpty);
    }
    return NULL;
}

void *f_watchdog(void *a)
{
    int fd, ret;
    int timeout = 0;
    static int Wdt_Complete_Init = 0;

    fd = open("/dev/watchdog", O_RDWR);             /* Open watchdog */
    if(fd < 0){
        fprintf(stderr, "Watchdog doesnt exist. Put into terminal - modprobe softdog - and run app with sudo \n");
        perror("fail");
        //exit(EXIT_FAILURE);
    }
    else{
        ret = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
        if(ret){
            fprintf(stderr, "timeout fail\n");
            perror("timeoutGet");
            exit(EXIT_FAILURE);
        }
        else{
            fprintf(stdout," Timeout %d sec \n", timeout);
            timeout = 120;  /* Timeout 120 = 2 min */
            ret = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
            if(ret){
                fprintf(stderr, "timeout fail\n");
                perror("timeoutGet");
                exit(EXIT_FAILURE);
            }
            fprintf(stdout," Obecny Timeout wynosi:  %d sec \n", timeout);
            puts(" Watchdog poprawnie skonfigurowany\n");
            Wdt_Complete_Init = 1;
        }
    }

    if(Wdt_Complete_Init)
    {
    //  if(wdt_sFlags.wdt_Analyzer || wdt_sFlags.wdt_Reader || wdt_sFlags.wdt_Printer || wdt_sFlags.wdt_Logger)
    //  {
            while(1){
                puts(" JESTEM W WATCHDOG \n ");
                sleep(3);
                ret = write(fd,"\0",1);
                if(ret != 1){
                    ret = -1;
                    //break;
                    printf("watchdog error write  %d \n", ret);
                    //close(fd);
                }
                else{
                    puts(" Hau \n");
                    wdt_sFlags.wdt_Reader = 0;
                    wdt_sFlags.wdt_Analyzer = 0;
                    wdt_sFlags.wdt_Printer = 0;
                    wdt_sFlags.wdt_Logger = 0;
                }
    //  }
                }
    } 
    return NULL;
}

void *f_logger(void *a)
{
    while(!done)
    {
    // puts(" JESTEM W LOGGER\n ");

    wdt_sFlags.wdt_Logger = 1;
    }
   
    return NULL;
}




int main(int argc, char *argv[])
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    if( sigaction(SIGTERM, &action, NULL) == -1)    printf(" Nie mozna utworzyć SIGTERM ");

    getCoreNumer(&sData);   /* Read how many cores system have */
    sem_init(&IsEmpty, 0, 1);
    sem_init(&IsFull, 0, 0);
    sem_init(&IsReady, 0, 0);

    pthread_t t_Reader,t_Analyzer,t_Printer,t_Watchdog,t_Logger,t_;
    

    if( pthread_create(&t_Reader, NULL, f_reader, NULL) == -1)      printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Analyzer, NULL, f_analiz, NULL) == -1)    printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Printer, NULL, f_print, NULL) == -1)      printf(" Nie mozna utworzyć wątku");
    //if( pthread_create(&t_Watchdog, NULL, f_watchdog, NULL) == -1)  printf(" Nie mozna utworzyć wątku");
    if( pthread_create(&t_Logger, NULL, f_logger, NULL) == -1)      printf(" Nie mozna utworzyć wątku");


    void *result;
    if(pthread_join(t_Reader, &result) == -1)   printf("Blad oczekiwania na zakonczenie watku tReader");
    if(pthread_join(t_Analyzer, &result) == -1) printf("Blad oczekiwania na zakonczenie watku t_Analyzer");
    if(pthread_join(t_Printer, &result) == -1)  printf("Blad oczekiwania na zakonczenie watku t_Printer");


    //if(pthread_join(t_Watchdog, &result) == -1) printf("Blad oczekiwania na zakonczenie watku t_Watchdog");

    if(pthread_join(t_Logger, &result) == -1)   printf("Blad oczekiwania na zakonczenie watku t_Logger");
    printf("done");
return 0;
}




