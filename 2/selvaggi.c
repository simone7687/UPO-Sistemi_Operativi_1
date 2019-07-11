#include <stdio.h>
#include <semaphore.h>  // per utilizare le system call POSIX per la gestione di semafori (sem_init, sem_wait, sem_post)
#include <pthread.h>    // per la gestione dei thread
#include <stdlib.h>

int N;  /* numero selvaggi */
int M;  /* numero porzioni */
int NGIRI;  /* numero giri */
int pentola;    /* pentola (puo' contenere fino ad M porzioni) */

Cuoco()
{
    while (1)
    {
        down(0);    // pentola vuota
        // <riempi pentola>
        up(M);      // pentola piena
    }
}
void Selvaggio()
{
    // Quando un selvaggio ha fame #6
    for (int i=0; i<NGIRI; i++)
    {
        // se non ci sono porzioni
        if (pentola == 0)
        {
            // sveglia il cuoco
            // attende che il cuoco abbia completamente
        }
        // se la pentola contiene almeno una porzione, se ne appropria
        if (pentola > 0)
        {
            pentola--;
        }
        // Ciascun selvaggio deve mangiare NGIRI #8
        else
        {
            i--;
        }
    }
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
}