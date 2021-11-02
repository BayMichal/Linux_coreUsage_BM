//----------------------------------------------------
//                  Michał Bajkos
//              Linux CPU Usage tracker 
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

#include <limits.h>     /* queque */
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
 
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
//#define F_QUEST             8   /* Quest        */
#define F_Q_NICE            9   /* Quest Nice   */
#define STAT_FIELDS         (F_USER + F_Q_NICE) /* 10 Stat Fields */
//#define WATCHDOG_TIMEOUT    120
#define RING_BUFFER_SIZE    1024



/**Typedef struct to get and prepare data to print thread
 * 
 */
typedef struct {
    unsigned int  core_count;                                                               /*Count of cores system have */
    char cpu_core[BUFOR_SIZE];                                                              /* Char buffer to get data from file */
    unsigned long long int Datebase[MAX_CORES][STAT_FIELDS]__attribute__ ((packed));        /* Datebase with converted and analyzed values */
    unsigned long long int prev_Datebase[MAX_CORES][STAT_FIELDS]__attribute__ ((packed)) ;  /* Datebase with previos step analyzed values */
    double CPU_Percentage[MAX_CORES]__attribute__ ((packed));
    
}dataStruct;

/**Typedef struct with data to analyze CPU usage
 * 
 */
typedef struct{
    unsigned long long int Idle;    
    unsigned long long int NonIdle;
    unsigned long long int Total;
}fieldStruct;

/**Typedef struct with data to analyze CPU usage. 
 * 
 */
typedef struct{
    unsigned long long int previdle;
    unsigned long long int prevNonIdle;
    unsigned long long int prevTotal;
}prev_fieldStruct;

/**Typedef struct to wachdog thread
 * 
 */
typedef struct{
    unsigned char wdt_Reader;   /* Flag drom Reader thread */
    unsigned char wdt_Analyzer; /* Flag from Analyzer thread */
    unsigned char wdt_Printer;  /* Flag from Printer thread */
    unsigned char wdt_Logger;   /* Flag from Logger thread */
}wdt_data;

/** Enum state for circle buffer
 * 
 * 
 */
typedef enum
{
	RB_OK       = 0,    /* Enum state True */
	RB_ERROR	= 1     /* Enum state Error*/
} RB_Status;
 

/** Global struct of circle buffer
 * 
 * 
 */
typedef struct
{
	int Head; // Pointer to write
	int Tail; // Pointer to read
	char Buffer[RING_BUFFER_SIZE]; // Array to store data
} RingBuffer_t;

static RingBuffer_t CircleBuf; 
static dataStruct sData ;
static fieldStruct sFieldData;
static prev_fieldStruct prev_sFieldData;
static wdt_data wdt_sFlags;    
static sem_t IsEmpty;          /* Bufor is empty, ready to read data */
static sem_t IsFull;           /* Bufor is full, ready to print */
static sem_t IsReady;          /* Bufor is print, ready to get empty */
static sem_t circle_empty;     /* Circle bufor is empty */
static sem_t circle_Full;      /* Circle buffer is full, now logger can work */


static char data[BUFOR_SIZE];   /* Create data char bufor */
static volatile sig_atomic_t done = 0; /* Variable sigterm */
static char error_name[] = "Error threads - Watchdog";
static int error_flag;


void getCoreNumer(dataStruct  *context);
void *f_reader(__attribute__((unused)) void *a);
void *f_analiz(__attribute__((unused)) void *a);
void *f_print(__attribute__((unused)) void *a);
void *f_softdog(__attribute__((unused)) void *a);
void *f_watchdog(__attribute__((unused)) void *a);
void *f_logger(__attribute__((unused)) void *a);
RB_Status RB_Read(RingBuffer_t *Buf, char *Value);
RB_Status RB_Write(RingBuffer_t *Buf, char Value);
void RB_Flush(RingBuffer_t *Buf);



/** Read from Ring Buffer
*
* @param[in] Buf  pointer to Ring Buffer structure
* @param[in] Value - pointer to place where a value from buffer is read
* 
*/
RB_Status RB_Read(RingBuffer_t *Buf, char *Value){

	/* Check if Tail hit Head */
	if(Buf->Head == Buf->Tail)
	{
		// If yes - there is nothing to read */
		return RB_ERROR;
	}
 
	/* Write current value from buffer to pointer from argument */
	*Value = Buf->Buffer[Buf->Tail];
 
	/* Calculate new Tail pointer */
	Buf->Tail = (Buf->Tail + 1) % RING_BUFFER_SIZE;
 
	/* Everything is ok - return OK status */
	return RB_OK;
}
 

/** Write to Ring Buffer
 *
 * @param[in] Buf - pointer to Ring Buffer structure
 * @param[in] Value - a value to store in the buffer
 */
RB_Status RB_Write(RingBuffer_t *Buf, char Value){
	
    /* Calculate new Head pointer value */
	int HeadTmp = (Buf->Head + 1) % RING_BUFFER_SIZE;
 
	/* Check if there is one free space ahead the Head buffer */
	if(HeadTmp == Buf->Tail)
	{
		/* There is no space in the buffer - return an error */
		return RB_ERROR;
	}
 
	/* Store a value into the buffer */
	Buf->Buffer[Buf->Head] = Value;
 
	/* Remember a new Head pointer value */
	Buf->Head = HeadTmp;
 
	/* Everything is ok - return OK status */
	return RB_OK;
}
 
/** Free whole circle buffer
 * 
 * @param Buf - Pointer to Ring Buffer structure
 */
void RB_Flush(RingBuffer_t *Buf){
	
	Buf->Head = 0;  //Reset Head
	Buf->Tail = 0;  //Reset Tail
}

/** Function read how many cores system have.
 * 
 * @param[in] context - pointer to global datastruct. 
 */
void getCoreNumer(dataStruct  *context){

    FILE *NMBR_CORES_N = fopen("/proc/cpuinfo" ,"r");   /* Create File pointer with command to read how many cores system have */
    int j = 0;
    while( fgets(data, BUFOR_SIZE, NMBR_CORES_N)  != NULL ){
        if (j == 11){
            context->core_count = ( (unsigned)data[ID_NBR_CORE] - '0');       /* Convert coure count from char to int */
            if(!context->core_count){
                puts("Error core count");
            }    
            break;
        }
        j++;
    }

    sem_post(&IsEmpty);
}
/** Function which use thread t_Reader
 * 
 * 
 */
void *f_reader(__attribute__((unused))void *a){

unsigned int rows_data = 0;                        /* variable describe number of row data from terminal */

int i = 0;
    while(!done){
    sem_wait(&circle_empty);
    const char log_buf[16] = "Reader_";

    for(int x = 0; x < 7; x++){
        RB_Write(&CircleBuf,log_buf[x]);
    }
    sem_post(&circle_Full);  

        
    sem_wait(&IsEmpty);                      /* Thread block */
    
        rows_data = 0;                        /* variable describe number of row data from terminal */
        i = 0;
            
        FILE *STAT = fopen("/proc/stat", "r");  /* Create File pointer with command to read file /proc/stat*/
        while( fgets(data, BUFOR_SIZE, STAT) != NULL){
            if(rows_data < sData.core_count ) {
                for(int j = 0; j < DATA_SIZE; j++){   
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

/** Function which use thread t_Analyzer
 * 
 * 
 */
void *f_analiz(__attribute__((unused))void *a){

    while(!done){
        
        sem_wait(&circle_empty);

        /* Log procedure */
        const char log_buf[10] = "Analyzer";

        for(int x = 0; x < 7; x++){
            RB_Write(&CircleBuf,log_buf[x]);
        }
        sem_post(&circle_Full);

        /* Wait for guards */
        sem_wait(&IsFull);                      
        
        unsigned int nbr_lopp_convert = 0;  /* Local var to increment fields of data */               
        static unsigned int id_Empty = 0;   /* Index of buffer which first empty char */
        unsigned int offset = 0;            /* Offset to global buffer */
        static char bufer[200];             /* Bufer to concencrate data char array -> int long */

        for(unsigned int x = 0; x < sData.core_count; x++)   /* Convert char buffer to Datebase[how many cores][ filds of data stat] */
        {
            offset = (100*x);
            nbr_lopp_convert = 0;
            id_Empty = 0;
        
            while(1){
                if(nbr_lopp_convert == 10)  {
                    break;
                }
                for(unsigned int i = 0; i < 100; i++){
                    bufer[i] = sData.cpu_core[i+id_Empty+offset]; 
                    if(sData.cpu_core[i+id_Empty+offset] != ' '){
                        continue;  /* If data from buffor is empty -> new data */
                    }
                    else{
                        id_Empty=id_Empty+i+1; 
                        break;
                    }
                }
                sData.Datebase[x][nbr_lopp_convert] = (unsigned long long)atoi(bufer); 
                memset(bufer,0,200);
                nbr_lopp_convert++;
            }   
        }

        /* Algorithm */
        unsigned long long int totald;
        unsigned long long int idled;

        for(unsigned int cnt_core = 0; cnt_core < sData.core_count; cnt_core++)
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
        wdt_sFlags.wdt_Analyzer = 1;

        /* Set semaphore */
        sem_post(&IsReady);
    }
    return NULL;

}

/** Function which use thread t_Printer
 * 
 * 
 */
void *f_print(__attribute__((unused))void *a){

    while(!done){

        sem_wait(&circle_empty);
        /* Circle procedure */
        const char log_buf[10] = "Printer";
        for(int x = 0; x < 7; x++){
            RB_Write(&CircleBuf,log_buf[x]);
        }
        sem_post(&circle_Full);


        /* Wait for guards */
        sem_wait(&IsReady);
        sleep(1);
        puts("\n");
        for(unsigned int k = 0; k < sData.core_count; k++){
            printf(" Zuzycie rdzenia nr: %d wynosi: %f %% \n", k, sData.CPU_Percentage[k]);
        }

        /* Prepare previos buffer for next step */
        memset(sData.prev_Datebase, 0 , sizeof(sData.prev_Datebase));
        memset(sData.CPU_Percentage, 0, sizeof(sData.CPU_Percentage));

        /* Save data to previos buffer for next use */
        memcpy(sData.prev_Datebase, sData.Datebase, sizeof(sData.Datebase));
        memset(sData.cpu_core, 0, sizeof(sData.cpu_core));
        wdt_sFlags.wdt_Printer = 1;

        /* Set semaphore */
        sem_post(&IsEmpty);
    }
    return NULL;
}

/* SOFTDOG HARDWAREA WATCHDOG 
 *
 *
 */
void *f_softdog(__attribute__((unused))void *a){
    
    int fd;
    long int ret;
    int timeout = 0;
    static int Wdt_Complete_Init = 0;

    sem_wait(&circle_empty);
    const char log_buf[10] = "Softdog";

    for(int x = 0; x < 7; x++){
        RB_Write(&CircleBuf,log_buf[x]);
    }
    sem_post(&circle_Full);


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
            while(1){
                puts(" JESTEM W WATCHDOG \n ");
                sleep(3);
                ret = write(fd,"\0",1);
                if(ret != 1){
                    ret = -1;
                    //break;
                    printf("watchdog error write  %ld \n", ret);
                    //close(fd);
                }
            }
    } 
    return NULL;
}


void *f_watchdog(__attribute__((unused))void *a){

    int timeout = 0;
    time_t TimeNoResponse_st = 0;
    time_t TimeNoResponse_end = 0;

    while(!done){

        sem_wait(&circle_empty);
        /* Circle procedure */
        const char log_buffr[10] = "Watchdg";
        
        for(int x = 0; x < 7; x++){
            RB_Write(&CircleBuf,log_buffr[x]);
        }
        sem_post(&circle_Full);
                
        if(timeout == 1) {      /* If we dont have response from threads */
            if((TimeNoResponse_end - TimeNoResponse_st) > 2){
                error_flag = 1;
                alarm(1);       /* Set alarm (1) */
                sleep(1);
            }
            TimeNoResponse_end = time(NULL);
        }
        
        if(wdt_sFlags.wdt_Analyzer == 1 || wdt_sFlags.wdt_Reader == 1 || wdt_sFlags.wdt_Printer == 1 || wdt_sFlags.wdt_Logger == 1){
            wdt_sFlags.wdt_Reader = 0;
            wdt_sFlags.wdt_Analyzer = 0;
            wdt_sFlags.wdt_Printer = 0;
            wdt_sFlags.wdt_Logger = 0;

            error_flag = 0;
            timeout = 0;
            TimeNoResponse_st = 0;
            alarm(0);       /* Clear alarm */
        }
        else if (timeout == 0){ 
            TimeNoResponse_st = time(NULL);
            timeout = 1;
        }
     }
    return NULL;
}


/** Function which use thread t_logger
 * 
 * 
 */
void *f_logger(__attribute__((unused)) void *a)
{
    char buf_ans[8];
    FILE *fp;
    while(!done){
        /* Guard of buffer */
        sem_wait(&circle_Full);
            
        /* Get timestamp for file */
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        char s[70];
        assert(strftime(s, sizeof(s), "%c", tm));

        /* READ FROM CIRCLE BUFFER */
        for(int z=0; z< 7; z++){
            RB_Read(&CircleBuf, &buf_ans[z]);
        }
        
        /* Save file */
        if ((fp=fopen("LOGS.txt", "a"))==NULL) {
            printf ("Nie mogę otworzyć pliku txt do zapisu!\n");
            }
        else{
            fprintf(fp, "%s", buf_ans);    //Which Tread
            fprintf(fp, "%s ", " ");        // space
            fprintf(fp, "%s \n", s);        // Timestamp
            fclose (fp); 
        }
        
        /* Prepare for next Log */
        memset(buf_ans,0,sizeof(buf_ans));
        RB_Flush(&CircleBuf);
        /* Set Empty semaphore guard */
        wdt_sFlags.wdt_Logger = 1;
        sem_post(&circle_empty);
    }
    return NULL;
}

/** Sigterm set Sigflag function 
 *  
 * @param signum - Handler
 */
static void term(__attribute__((unused)) int signum){
    //signum=signum;
    done = 1;
    if(error_flag){
        printf("\n %s ", error_name);
        error_flag = 0;
    }
    else{
        puts("\n Nastąpi zakończenie programu \n ");
    }
}

int main(void)
{
    /* SIGTERM */
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Weverything"     /* Ignore -wno-disable-macro-expansion flag in -Weverything. Only for this handler */
        action.sa_handler = term;
        #pragma clang diagnostic pop
    #else
        action.sa_handler = term;
    #endif
    
    if( sigaction(SIGTERM, &action, NULL) == -1){
        printf(" Nie mozna utworzyć obsługi SIGTERM ");
    }    
    signal(SIGALRM,term);

    /* SEMAPHORES */
    sem_init(&IsEmpty, 0, 1);
    sem_init(&IsFull, 0, 0);
    sem_init(&IsReady, 0, 0); 
    sem_init(&circle_empty,0,1);
    sem_init(&circle_Full,0,0);
 

    /* CIRCLE BUFFER INIT */
    RB_Flush(&CircleBuf);

    /* THREADS INIT */
    pthread_t t_Reader,t_Analyzer,t_Printer,t_Watchdog,t_Logger /*,t_SoftDog*/;

    /* Read how many cores system have */
    getCoreNumer(&sData);   

    /* THREADS CREATE */
    if( pthread_create(&t_Reader, NULL, f_reader, NULL) == -1){
        printf(" Nie mozna utworzyć wątku");
        }

    if( pthread_create(&t_Analyzer, NULL, f_analiz, NULL) == -1){
        printf(" Nie mozna utworzyć wątku");
        }

    if( pthread_create(&t_Printer, NULL, f_print, NULL) == -1){
        printf(" Nie mozna utworzyć wątku");
        }  

    //if( pthread_create(&t_SoftDog, NULL, f_softdog, NULL) == -1){  /* If you want use Hardware watchdog -> uncomment */
    //   printf(" Nie mozna utworzyć wątku");
    //}

    if( pthread_create(&t_Watchdog, NULL, f_watchdog, NULL) == -1){
        printf(" Nie mozna utworzyć wątku");
        }

    if( pthread_create(&t_Logger, NULL, f_logger, NULL) == -1){
        printf(" Nie mozna utworzyć wątku");
        }     
     

    /* THREADS EXIT */
    void *result;
    if(pthread_join(t_Reader, &result) == -1){
        printf("Blad oczekiwania na zakonczenie watku tReader");
    }   
    if(pthread_join(t_Analyzer, &result) == -1){
        printf("Blad oczekiwania na zakonczenie watku t_Analyzer");
    } 
    if(pthread_join(t_Printer, &result) == -1){
        printf("Blad oczekiwania na zakonczenie watku t_Printer");
    }  
    //if(pthread_join(t_SoftDog, &result) == -1){
    //    printf("Blad oczekiwania na zakonczenie watku t_Printer");
   // }
    if(pthread_join(t_Watchdog, &result) == -1){
        printf("Blad oczekiwania na zakonczenie watku t_Watchdog");
    }
    if(pthread_join(t_Logger, &result) == -1){
        printf("Blad oczekiwania na zakonczenie watku t_Logger");
    }

puts(" \n the end ");
return 0;
}



