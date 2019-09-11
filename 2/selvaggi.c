#include <stdio.h>
#include <semaphore.h>  // per utilizare le system call POSIX per la gestione di semafori (sem_init, sem_wait, sem_post)
#include <pthread.h>    // per la gestione dei thread
#include <stdlib.h>
#include <unistd.h>     // sleep

int N;  /* numero selvaggi */
int M = 0;  /* numero porzioni */
int NGIRI;  /* numero giri */
int pentola;    /* pentola (puo' contenere fino ad M porzioni) */
int K = 0;  /* pentole riempite */
sem_t vuoto; /* semaforo vuoto */
sem_t pieno; /* semaforo pieno */
sem_t mutex; /* semaforo sessione critica */
int i;

void * Cuoco()
{
    pentola = M;        // riempi pentola
    printf("Cuoco ha RIEMPITO la pentola\n");
    while (1)
    {
        printf("Cuoco e' andato a DORMIRE\n");
        sem_wait(&vuoto);   // richiede una pentola vuota   down -> wait
        sem_wait(&mutex);   // entra in sezione critica
        pentola = M;        // riempi pentola
        K++;
        printf("Cuoco ha RIEMPITO la pentola\n");
        sem_post(&mutex);   // esce dalla sezione critica
        sem_post(&pieno);   // rilascia una pentola piena   up -> signal
    }
    pthread_exit(NULL);
}
void * Selvaggio()
{
    int id = i++;   //= pthread_self();
    // Quando un selvaggio ha fame #6
    // Ciascun selvaggio deve mangiare NGIRI #8
    for (int i=0; i<NGIRI; i++)
    {
        printf("Entra in sezione critica\n");
        sem_wait(&mutex);   // entra in sezione critica
        // se non ci sono porzioni
        if (pentola == 0)
        {            
            sem_post(&vuoto);   // rilascia una pentola vuota   up -> signal
            sem_wait(&pieno);   // richiede una pentola piena   down -> wait
            printf("Selvaggio %d ha SVEGLIATO il cuoco\n",id);
        }
        // se la pentola contiene almeno una porzione, se ne appropria
        pentola--;          // prendi una porzione dalla pentola
        printf("Selvaggio %d ha MANGIATO, per essere sazio mancano: %d\n", id, NGIRI-i-1);
        printf("Porzioni rimanenti: %d\n", pentola);
        printf("Esce in sezione critica\n");
        sem_post(&mutex);   // esce dalla sezione critica
        sleep(1);   // selvaggio aspetta che gli altri prendono al loro porzione
    }
    printf("Selvaggio %d e' SAZIO\n", id);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
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
    // semafori (lo 0 nel seconda "colonna": il semaforo è condiviso tra thread (uguale a 0) o processi (diverso da 0))
    sem_init(&pieno, 0, M);
    sem_init(&vuoto, 0, 0);
    sem_init(&mutex, 0, 1);
    // thread cuoco
    pthread_t cuoco;
    if (pthread_create(&cuoco, NULL, Cuoco, NULL))
    {
        perror("thread cuoco");
    }
    // thread selvaggi
    pthread_t selvaggi[N];
    for (int i=0; i<N; i++)
    {
        if (pthread_create(&selvaggi[i], NULL, Selvaggio, NULL))
        {
            perror("thread selvaggi");
        }
    }
    // Il programma termina quando tutti i selvaggi hanno completato il loro ciclo (selvaggi) #11
    for (int i=0; i<N; i++)
    {
        if (pthread_join(selvaggi[i], NULL))
        {
            perror("attende i selvaggi");
        }
    }
    // Contare quante volte il cuoco riempie la pentola (selvaggi) #10
    printf("Pentole riempite: %d\n", K);    // eccetto la prima volta
}