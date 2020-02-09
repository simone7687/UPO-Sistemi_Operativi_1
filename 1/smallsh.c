#include "smallsh.h"
#include <sys/types.h>
#include <signal.h> 
#include <fcntl.h>  // tipi di apertura file
#include <assert.h> // assert
#include <string.h>

char prompt[MAXBUF];

int fd = 0; /* file */
pid_t pid;
void sigint_handler (int sig);  /* CTRL-C */
void wait_child();   /* Aspetta eventuali figlio morti e informazioni sul fatto che il comando è terminato #2 */
// Tenere traccia tramite una variabile d’ambiente BPID (smallsh) #18
void print_pid(const char * name);  /* stampa i pid */
void add_pid(int x);    /* aggiunge i pid */
void remove_pid(int x);

void sigint_handler(int sig);   /* chiude i processi in background attraverso CTR-C */

int procline(void) 	/* tratta una riga di input */
{
    char *arg[MAXARG+1];	/* array di puntatori per runcommand */
    int toktype;  	/* tipo del simbolo nel comando */
    int narg;		/* numero di argomenti considerati finora */
    int type;		/* FOREGROUND o BACKGROUND */
    // Redirezione dello standard ouput su un file #4
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
    int exitstat,ret;
    // L'interprete deve ignorare il segnale di interruzione solo quando è in corso un comando in foreground #3
    if(where == FOREGROUND)
    {
        signal(SIGINT, SIG_IGN);    // I segnali SIGKILL e SIGSTOP non possono essere ne' ignorati e ne' catturati
    }
    pid = fork();
    // Tenere traccia tramite una variabile d’ambiente BPID (smallsh) #18
    if (pid != (pid_t) 0)
        add_pid(pid);
    
    if (pid == (pid_t) -1) 
    {
        perror("smallsh: fork fallita");
        return;
    }
    if (pid == (pid_t) 0)           /* processo figlio */
    {
        // Informazioni sul fatto che il comando è terminato (smallsh) #2
        if(where == BACKGROUND)
        {
            printf("\n\nprocesso background [%d]\n", getpid());
        }
        /* esegue il comando il cui nome e' il primo elemento di cline,
        passando cline come vettore di argomenti */

        // Tenere traccia tramite una variabile d’ambiente BPID (smallsh) #18
        if (strcmp(*cline, "bp") == 0)
        {
            print_pid("BPID");
            exit(1);
        }
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
        ret = waitpid(pid, &exitstat, 0);
        if (ret == -1) perror("wait");
        // Tenere traccia tramite una variabile d’ambiente BPID (smallsh) #18
        remove_pid(ret);
        // Possibilità di interrompere un comando #3
        signal(SIGINT, sigint_handler); // il segnale CTRL-C svolge sigint_handler
    }
    // chiusura file #4
    if(fd != 0)
        close(fd);
}
// Possibilità di interrompere un comando #3
void sigint_handler(int sig)
{
    wait_child();
}
void wait_child()
{
    int exitstat, ret=1;
    while(ret !=-1 && ret !=0)
    {
        ret = waitpid(-1, &exitstat, WNOHANG);  //WNOHANG:specifica il ritorno immediato se i child non sono usciti.
        if (ret > 0)    //-1: errore e 0: non ci sono figli che hanno terminato
        {
            // Informazioni sul fatto che il comando è terminato (smallsh) #2
            if (WTERMSIG(exitstat) == SIGINT)   // WTERMSIG: riporta il segnale che ha causato il termine del processo figlio
                printf("\nprocesso terminato con CTRL-C [%d]\n", ret);
            else
                printf("\nprocesso terminato [%d]\n", ret);
            // Tenere traccia tramite una variabile d’ambiente BPID (smallsh) #18
            remove_pid(ret);
        }
    }
}

// Tenere traccia tramite una variabile d’ambiente BPID (smallsh) #18
void print_pid(const char * name)
{
    char * value;
    value = getenv (name);
    if (! value)
        printf("%s:nullo.\n", name);
    else
        printf("%s%s\n", name, value);
}
void add_pid(int x)
{
    char pid[10];
    char * value;
    char buffer[MAXBUF];
    memset(&pid, '\0', sizeof(pid));
    memset(&value, '\0', sizeof(value));
    memset(&buffer, '\0', sizeof(buffer));
    sprintf(pid, "%d", x);
    value = getenv ("BPID");
    strcat(buffer, value);
    strcat(buffer, ":");
    strcat(buffer, pid);
    setenv("BPID", buffer, sizeof(buffer));
}
void remove_pid(int x)
{
    char pid[10];
    char buf[200];
    char * value = getenv ("BPID");
    memset(&buf, '\0', sizeof(buf));
    sprintf(pid, "%d", x);

    char *token = strtok(value, ":");
    while (token != NULL)
    {
        if(strncmp(token, pid, strlen(pid)) != 0)
        {
            strcat(buf, ":");
            strcat(buf, token);
        }
        token = strtok(NULL, ":");
    }
    setenv("BPID", buf, sizeof(buf));
}

int main()
{
    setenv("BPID", "", 0);
    // Possibilità di interrompere un comando #3
    signal(SIGINT, sigint_handler); // il segnale CTRL-C svolge sigint_handler
    // Incitare a dare un comando (smallsh) #17
    prompt[0] = '%';
    strncat(prompt, getenv("USER"),15);     //windows: USERPROFILE
    strcat(prompt, ":");
    strncat(prompt, getenv("HOME"), 255);   //directory: PATH

    while(userin(prompt) != EOF)
    {
        wait_child();
        procline();
    }
}
