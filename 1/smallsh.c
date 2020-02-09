#include "smallsh.h"
#include <sys/types.h>
#include <signal.h> 
#include <fcntl.h>  // tipi di apertura file
#include <assert.h> // assert
#include <string.h>

char prompt[MAXBUF];

int fd = 0; /* file */
pid_t pid;
void sigint_handler (int sig);
void wait_child();
void print_pid(const char * ev_name);
void add_pid(int pid_int);
void remove_pid(int pid_int);

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

// Esegue un comando (cline), where: foreground o background
void runcommand(char **cline,int where)
{
    int exitstat,ret;
    // L'interprete deve ignorare il segnale di interruzione solo quando è in corso un comando in foreground #3
    if(where == FOREGROUND)
    {
        signal(SIGINT, sigint_handler); // quando il processo corrente riceve il segnale SIGINT, il controllo deve eseguire sigint_handler
    }
    pid = fork();
    // Aggiungo il pid alla variabile d’ambiente BPID #18
    if (pid != (pid_t) 0)
        add_pid(pid);
    
    if (pid == (pid_t) -1) 
    {
        perror("smallsh: fork fallita");
        return;
    }
    if (pid == (pid_t) 0)   // processo figlio
    {
        // Informazioni sul fatto che il comando background avviato #2
        if(where == BACKGROUND)
        {
            printf("\n\nprocesso background [%d]\n", getpid());
        }
        /* esegue il comando il cui nome e' il primo elemento di cline,
        passando cline come vettore di argomenti */

        // Stampo la variabile d’ambiente BPID #18
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

        execvp(*cline,cline);   // *cline: comando (primo elemento array), cline: array completo compreso NULL 
        perror(*cline);
        exit(1);
    }
    // Foreground commands #1
    else if(where == FOREGROUND)    // processo padre
    {
        ret = waitpid(pid, &exitstat, 0);
        if (ret == -1) perror("wait");
        // Rimuovo il pid dalla variabile d’ambiente BPID #18
        remove_pid(ret);
        // Possibilità di interrompere un comando #3
        signal(SIGINT, SIG_DFL);    // il segnale CTRL-C svolge segnale di default
    }
    // chiusura file #4
    if(fd != 0)
        close(fd);
}
// Segnale CTRL-C Foregound #3
void sigint_handler(int sig)
{
    wait_child();
}
// Aspetta eventuali figli zombie e stampa eventuali informazioni #2
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
            // Rimuovo il pid dalla variabile d’ambiente BPID #18
            remove_pid(ret);
        }
    }
}

// Stampa la variabile d’ambiente ev_name #18
void print_pid(const char * ev_name)
{
    char * value;
    value = getenv (ev_name);
    if (! value)
        printf("%s:nullo.\n", ev_name);
    else
        printf("%s%s\n", ev_name, value);
}
// Aggiunge il pid_int alla variabile d’ambiente BPID #18
void add_pid(int pid_int)
{
    char pid[10];
    char * value;
    char buffer[MAXBUF];
    memset(&pid, '\0', sizeof(pid));
    memset(&value, '\0', sizeof(value));
    memset(&buffer, '\0', sizeof(buffer));
    sprintf(pid, "%d", pid_int);
    value = getenv ("BPID");
    strcat(buffer, value);
    strcat(buffer, ":");
    strcat(buffer, pid);
    setenv("BPID", buffer, sizeof(buffer));
}
// Rimuove il pid_int dalla variabile d’ambiente BPID #18
void remove_pid(int pid_int)
{
    char pid[10];
    char buf[200];
    char * value = getenv ("BPID");
    memset(&buf, '\0', sizeof(buf));
    sprintf(pid, "%d", pid_int);

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
