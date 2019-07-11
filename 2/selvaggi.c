#include <stdio.h>
#include <semaphore.h>  // per utilizare le system call POSIX per la gestione di semafori (sem_init, sem_wait, sem_post)
#include <pthread.h>    // per la gestione dei thread
#include <stdlib.h>

int N;  /* numero selvaggi */
int M;  /* numero porzioni */
int NGIRI;  /* numero giri */
int pentola;    /* pentola (puo' contenere fino ad M porzioni) */

void * Cuoco()
{
    while (1)
    {
        down(0);    // pentola vuota
        // <riempi pentola>
        up(M);      // pentola piena
    }
    pthread_exit(NULL);
}
void * Selvaggio()
{
    int id = pthread_self();
    // Quando un selvaggio ha fame #6
    for (int i=0; i<NGIRI; i++)
    {
        // se non ci sono porzioni
        if (pentola == 0)
        {
            printf("La pentola e' VUOTA\n");
            // sveglia il cuoco
            // attende che il cuoco abbia completamente
        }
        // se la pentola contiene almeno una porzione, se ne appropria
        if (pentola > 0)
        {
            pentola--;
            printf("Selvaggio %d ha MANGIATO\n", id);
        }
        // Ciascun selvaggio deve mangiare NGIRI #8
        else
        {
            i--;
            printf("Selvaggio %d e' stato LENTO\n", id);
        }
        printf("Al selvaggio %d mancano ancora %d porzioni\n", id, i);
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
    
    pentola = M;
    // thread cuoco
    pthread_t cuoco;
    if (pthread_create(&cuoco, NULL, Selvaggio, NULL))
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
    // attende i selvaggi
    for (int i=0; i<N; i++)
    {
        if (pthread_join(selvaggi[i], NULL))
        {
            perror("attende i selvaggi");
        }
    }
}