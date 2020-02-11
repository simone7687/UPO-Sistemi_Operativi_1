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

#define mutex 0
#define vouto 1
#define pieno 2

int N;          /* numero selvaggi */
int M = 0;      /* numero porzioni */
int NGIRI;      /* numero giri */
int shmid;
int semid;
void clear(int s);

struct buffer
{
    int pentola;    /* pentola (puo' contenere fino ad M porzioni) */
    int K;          /* pentole riempite */
} *buf;	        	/* puntatore; lo spazio per il buffer viene allocato in memoria condivisa */

void Cuoco()
{
    while (1)
    {
        down(semid, vouto);      // richiede una pentola vuota
        buf->pentola = M;   // riempi pentola
        buf->K += 1;
        printf("\nCuoco ha RIEMPITO la pentola\n");
        printf("Cuoco è andato a dormire\n\n");
        up(semid, pieno);        // rilascia una pentola piena
        sleep(1);
    }
}

void Selvaggio(int id)
{
    // Quando un selvaggio ha fame #6
    // Ciascun selvaggio deve mangiare NGIRI #8
    for (int i=0; i<NGIRI; i++)
    {
        down(semid, mutex);      // entra in sezione critica
        printf("Selvaggio %d è ENTRATO in sezione critica\n", id);
        // se non ci sono porzioni
        if(buf->pentola == 0)
        {
            printf("Selvaggio %d ASPETTA il cuoco\n", id);
            up(semid, vouto);    // rilascia una pentola vuota
            down(semid, pieno);  // richiede una pentola piena
        }
        // se la pentola contiene almeno una porzione, se ne appropria
        if(buf->pentola > 0)
        {
            buf->pentola--;
            printf("Selvaggio %d ha MANGIATO, per essere sazio mancano: %d\n", id, NGIRI-i-1);
            printf("Porzioni rimanenti: %d\n", buf->pentola);
        }
        up(semid, mutex);      // esce dalla sezione critica
        printf("Selvaggio %d ESCE della sezione critica\n", id);
        sleep(1);
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
        if ((semid = semget(IPC_PRIVATE, 3, 0666)) == -1)   // 3: dimensione vettore di semafori, 0666: accesso read /write  
            perror("semget");
        seminit(semid, mutex, 1);
        seminit(semid, vouto, 0);
        seminit(semid, pieno, 0);

    // Memoria condivisa
        if ((shmid = shmget(IPC_PRIVATE, sizeof(struct buffer), 0666))==-1)
            perror("shmget");
        buf = (struct buffer *) shmat(shmid,0,0);
        if (buf == (void *)-1) perror("shmat");

        sa.sa_handler = clear;
        sigaction(SIGINT, &sa, NULL);

        buf->pentola = M;
        buf->K = 0;

    // Creazione processi
        fflush(stdout);

        // processo cuoco
        pid_cuoco = fork();
        if (pid_cuoco == (pid_t) 0)
            Cuoco(argv[1]);

        // processi selvaggi
        for(int i=0; i<N; i++)
            if (fork()==0)
            {
                Selvaggio(i+1);
                exit(0);
            }

    // Attesa processi
        for(int i=0; i<N; i++)
            pid = wait(NULL);        
        kill(pid_cuoco, 1);
        wait(NULL);
        
    // Contare quante volte il cuoco riempie la pentola (selvaggi) #10
        printf("Pentole riempite: %d\n", buf->K);   // eccetto la prima volta
        printf("Porzioni rimanenti: %d\n", buf->pentola);

        clear(0);
}

// rimuove strutture IPC
void clear(int s)
{
    if (semctl(semid, 0, IPC_RMID) == -1) perror("semctl");
    if (shmctl(shmid, IPC_RMID, 0) == -1) perror("shmctl");
    exit(s);
}