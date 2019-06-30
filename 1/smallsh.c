#include "smallsh.h"
#include <sys/types.h>

char *prompt = "Dare un comando>";

int procline(void) 	/* tratta una riga di input */
{
    char *arg[MAXARG+1];	/* array di puntatori per runcommand */
    int toktype;  	/* tipo del simbolo nel comando */
    int narg;		/* numero di argomenti considerati finora */
    int type;		/* FOREGROUND o BACKGROUND */

    narg=0;

    while (1) /* ciclo da cui si esce con il return */
    {
        /* esegue un'azione a seconda del tipo di simbolo */

        /* mette un simbolo in arg[narg] */

        switch (toktype = gettok(&arg[narg])) 
        {
            /* se argomento: passa al prossimo simbolo */
            case ARG:

            if (narg < MAXARG)
                narg++;
                break;

            /* se fine riga o ';' o '&' esegue il comando ora contenuto in arg,
            mettendo NULL per segnalare la fine degli argomenti: serve a execvp */
            case EOL:
            case SEMICOLON:
            case AMPERSAND:
            type = (toktype == AMPERSAND) ? BACKGROUND : FOREGROUND;

            if (narg != 0) 
            {
                arg[narg] = NULL;
                runcommand(arg,type);
            }

            /* se fine riga, procline e' finita */

            if (toktype == EOL) return 1;

            /* altrimenti (caso del comando terminato da ';' o '&') 
            bisogna ricominciare a riempire arg dall'indice 0 */

            narg = 0;
            break;
        }
    }
}

void runcommand(char **cline,int where)	/* esegue un comando */
{
    pid_t pid;
    int exitstat,ret;

    pid = fork();
    if (pid == (pid_t) -1) 
    {
        perror("smallsh: fork fallita");
        return;
    }

    if (pid == (pid_t) 0) /* processo figlio */
    {
        /* esegue il comando il cui nome e' il primo elemento di cline,
        passando cline come vettore di argomenti */

        execvp(*cline,cline);
        perror(*cline);
        exit(1);
    }
    // Background commands (&) #1
    else if(where != BACKGROUND) /* processo padre */
    {
        ret = wait(&exitstat);

        if (ret == -1) perror("wait");
    }
}

int main()
{
    while(userin(prompt) != EOF)
    procline();
}
