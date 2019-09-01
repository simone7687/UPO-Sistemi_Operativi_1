#include "smallsh.h"
#include <sys/types.h>
#include <signal.h> 
#include <fcntl.h>  // tipi di apertura file
#include <assert.h> // assert

char *prompt = "Dare un comando>";
void sigint_handler(int sig_num);
int fd = 0; /* file */

int procline(void) 	/* tratta una riga di input */
{
    char *arg[MAXARG+1];	/* array di puntatori per runcommand */
    int toktype;  	/* tipo del simbolo nel comando */
    int narg;		/* numero di argomenti considerati finora */
    int type;		/* FOREGROUND o BACKGROUND */
    fd = 0;

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
                {narg++;}
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
            // Redirezione dello standard ouput su un file #4
            case REDIRECT:
            if((fd = open(pathname, O_CREAT|O_WRONLY|O_TRUNC)) < 0) // se non esiste viene creato, solo scrittura, file esistente verra' troncato = creat()
            {
                perror("creat");
                exit(1);
            }
            break;
            // Redirezione dello standard ouput su un file in modalità APPEND #5
            case APPEND:
            if((fd = open(pathname, O_CREAT|O_APPEND|O_WRONLY)) < 0)    // successive operazioni di scrittura verranno accodate 
            {
                perror("open");
                exit(1);
            }
            break; 
        }
    }
}

void runcommand(char **cline,int where)	/* esegue un comando */
{
    pid_t pid1, pid2;
    int exitstat,ret;

    // L'interprete deve ignorare il segnale di interruzione solo quando è in corso un comando in foreground #3
    if (where == FOREGROUND)
    {
        signal(SIGINT, SIG_IGN);    // I segnali SIGKILL e SIGSTOP non possono essere ne' ignorati e ne' catturati
    }

    pid1 = fork();
    if (pid1 == (pid_t) -1) 
    {
        perror("smallsh: fork fallita");
        return;
    }
    if (pid1 == (pid_t) 0)  /* processo figlio */
    {
        /* esegue il comando il cui nome e' il primo elemento di cline,
        passando cline come vettore di argomenti */

        // Redirezione dello standard ouput su un file #4
        if (fd != 0)
        {
            lseek(fd, 0L, SEEK_END);    // La nuova posizione e' calcolata aggiungendo offset dalla fine file.
            ret = dup2(fd, 1);  // crea newfd come copia di oldfd, chiudendo prima newfd se e' necessario (int oldfd, int newfd)
            if (ret < 0)
            {
                perror("dup2");
                exit(1);
            }
        }

        execvp(*cline,cline);
        perror(*cline);
        exit(1);
    }
    // Background commands (&) #1
    else if(where == FOREGROUND)    /* processo padre */
    {
        ret = waitpid(pid1, &exitstat, 0);
        if (ret == -1) perror("wait");
    }
    else if(where == BACKGROUND)    /* processo padre */
    {
        // Informazioni sul fatto che il comando è terminato #2
        pid2 = fork();
        if (pid2 == (pid_t) -1) 
        {
            perror("smallsh: fork fallita");
            return;
        }
        if (pid2 != (pid_t) 0)  /* processo padre */
        {
            ret = waitpid(pid1, &exitstat, 0);
            if (ret == -1) perror("wait");
            printf("\nEsecuzione terminata\n\nDare un comando> ");
        }
    }
    // Informazioni sul fatto che il comando è terminato #2
    if(pid2 != (pid_t) 0)
    {
        ret = waitpid(pid2, &exitstat, WNOHANG);
        ret = waitpid(pid2, &exitstat, 0);
        // chiusura file #4
        if(fd != 0)
        {
            close(fd);
        }
    }
    // L'interprete deve ignorare il segnale di interruzione solo quando è in corso un comando in foreground #3
    if (where == FOREGROUND)
    {
        signal(SIGINT, sigint_handler);
    }
}

pid_t parent_pid;
void sigquit_handler (int sig)
{
    assert(sig == SIGQUIT);
    pid_t self = getpid();
    if (parent_pid != self) _exit(0);
}
void sigint_handler(int sig_num)    /* invia segnale di chiusura ad ogni processo eccetto per il primo */
{
    kill(-parent_pid, SIGQUIT);
} 

int main()
{
    // Possibilità di interrompere un comando #3
    signal(SIGQUIT, sigquit_handler);
    parent_pid = getpid();
    signal(SIGINT, sigint_handler); // il segnale CTRL-C svolge sigint_handler
    fflush(stdout);

    while(userin(prompt) != EOF)
    procline();
}
