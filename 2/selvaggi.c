#include <stdio.h>
#include <semaphore.h>  // per utilizare le system call POSIX per la gestione di semafori (sem_init, sem_wait, sem_post)
#include <pthread.h>    // per la gestione dei thread

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
Selvaggio()
{
    for (int i=0; i<NGIRI; i++)
    {
        // <pensa>
        // …
        if (pentola == 0)
        {
            // …
        }
        pentola--;
        // …
        // <mangia porzione>
    }
}

int main()
{
    pentola = M;
}