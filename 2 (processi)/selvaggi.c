#include <stdio.h>
#include <semaphore.h>  // per utilizare le system call POSIX per la gestione di semafori (sem_init, sem_wait, sem_post)
#include <stdlib.h>
#include <unistd.h>     // sleep

#include "semfun.h"
#include <signal.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

int N;          /* numero selvaggi */
int M = 0;      /* numero porzioni */
int NGIRI;      /* numero giri */
int vuoto;
int pieno;
int mutex;
int shmid;
void clear(int s);  /* rimuove strutture IPC */

struct buffer
{
    int pentola;    /* pentola (puo' contenere fino ad M porzioni) */
    int K;          /* pentole riempite */
} *buf;	        	/* puntatore; lo spazio per il buffer viene allocato in memoria condivisa */

void Cuoco()
{
    while (1)
    {
        down(vuoto,0);      // richiede una pentola vuota
        buf->pentola = M;   // riempi pentola
        buf->K += 1;
        printf("Cuoco ha RIEMPITO la pentola\n");
        up(pieno,0);        // rilascia una pentola piena
        printf("Cuoco e' andato a DORMIRE\n");
    }
}

void Selvaggio(int id)
{
    // Quando un selvaggio ha fame #6
    // Ciascun selvaggio deve mangiare NGIRI #8
    for (int i=0; i<NGIRI; i++)
    {
        down(mutex,0);      // entra in sezione critica
        printf("Selvaggio %d ENTRA in sezione critica\n", id);
        // se non ci sono porzioni
        if(buf->pentola == 0)
        {
            printf("Selvaggio %d ASPETTA cuoco\n", id);
            up(vuoto,0);    // rilascia una pentola vuota
            down(pieno,0);  // richiede una pentola piena
        }
        // se la pentola contiene almeno una porzione, se ne appropria
        if(buf->pentola > 0)
        {
            buf->pentola--;
            printf("Selvaggio %d ha MANGIATO, per essere sazio mancano: %d\n", id, NGIRI-i-1);
            printf("Porzioni rimanenti: %d\n", buf->pentola);
        }
        up(mutex,0);      // esce dalla sezione critica
        printf("Selvaggio %d ESCE della sezione critica\n", id);
    }
    printf("Selvaggio %d e' SAZIO\n", id);
}

int main(int argc, char *argv[])
{
    pid_t pid_cuoco, pid;
    struct sigaction sa;
    // Il numero di selvaggi, di porzioni e di giri dovranno essere richiesti come argomenti #9
        if (argc != 4)
        {
            fprintf(stderr, "%s <numero_selvaggi> <numero_porzioni> <numero_giri>'\n", argv[0]);
            exit(1);
        }
        N = atoi(argv[1]);
        M = atoi(argv[2]);
        NGIRI = atoi(argv[3]);
        printf("numero selvaggi = %d  \nnumero porzioni = %d  \nnumero giri     = %d\n", N, M, NGIRI);

    // Semafori (lo 0 nel seconda "colonna": il semaforo è condiviso tra thread (uguale a 0) o processi (diverso da 0))
        if ((mutex = semget(IPC_PRIVATE, 1, 0666)) == -1)
        perror("semget");
        seminit(mutex, 0, 1);

        if ((vuoto = semget(IPC_PRIVATE, 1, 0666)) == -1)
        perror("semget");
        seminit(vuoto, 1, N);

        if ((pieno = semget(IPC_PRIVATE, 1, 0666)) == -1)
        perror("semget");
        seminit(pieno, 0, 0);

    // Memoria condivisa
        if ((shmid = shmget(IPC_PRIVATE,sizeof(struct buffer),0666))==-1)
        perror("shmget");
        buf = (struct buffer *) shmat(shmid,0,0);
        if (buf == (void *)-1) perror("shmat");

        sa.sa_handler = clear;
        sigaction(SIGINT,&sa,NULL);

        buf->pentola = M;
        buf->K = 0;

    // Creazione processi
        fflush(stdout);

        // processo cuoco
        pid_cuoco = fork();
        if (pid_cuoco == (pid_t) 0)
        {Cuoco(argv[1]);}

        // processi selvaggi
        for(int i=0; i<N; i++)
        {
            if (fork()==0)
            {Selvaggio(i+1);   exit(0);}
        }

    // Attesa processi
        for(int i=0; i<N; i++)
        {
            pid = wait(NULL);
        }

        // Contare quante volte il cuoco riempie la pentola (selvaggi) #10
        kill(pid_cuoco, 1);
        wait(NULL);
        printf("Pentole riempite: %d\n", buf->K);   // eccetto la prima volta
        printf("Porzioni rimanenti: %d\n", buf->pentola);

        clear(0);
}

void clear(int s)
{
    if (semctl(mutex, 0, IPC_RMID) == -1) perror("semctl");
    if (semctl(pieno, 0, IPC_RMID) == -1) perror("semctl");
    if (semctl(vuoto, 0, IPC_RMID) == -1) perror("semctl");
    if (shmctl(shmid, IPC_RMID, 0) == -1) perror("shmctl");
    exit(s);
}