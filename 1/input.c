#include "smallsh.h"
#include <string.h>

/* buffers per la riga di input e la sua segmentazione in "tokens";
puntatori per scorrere i buffers */

static char inpbuf[MAXBUF], tokbuf[2*MAXBUF], *ptr, *tok;
// Redirezione dello standard ouput su un file #4
int file_path();

/* array di caratteri che hanno una interpretazione "speciale" 
nei comandi */

static char special[]=
{' ', '\t', '&', ';', '\n', '\0', '>'};

int userin(char *p) /* stampa il prompt e legge una riga */
{
    int c, count;

    /* inizializzazioni per altre routines */

    ptr = inpbuf;
    tok = tokbuf;

    /* stampa il prompt */

    printf("%s ",p);

    count=0;

    while(1) 
    {

        if ((c = getchar()) == EOF)
            return(EOF);

        /* si copia il carattere letto in inpbuf; ma se si raggiunge 
        e supera MAXBUF, non si scrive piu' in inpbuf, 
        si continua a leggere fino a newline (si veda sotto) */

        if (count < MAXBUF)
            inpbuf[count++] = c;

        /* se si legge il newline, la riga in input e' finita */

        if (c == '\n' && count < MAXBUF) 
        {
            inpbuf[count] = '\0';
            return(count);
        }

        /*  se e' stato superato MAXBUF, quando si arriva al newline
        si avverte che la riga e' troppo lunga e si 
        va a leggere una nuova riga */

        if (c == '\n')  /* implicito se si arriva qui: count >= MAXBUF */
        {
            printf("riga in input troppo lunga\n");
            count = 0;
            printf("%s ",p);
        }
    }
}

int gettok(char **outptr)   /* legge un simbolo e lo mette in tokbuf */
{
    int type;

    /* si piazza *outptr in modo che punti al primo byte dove si cominicera'
    a scrivere il simbolo letto */  

    *outptr = tok;

    /* salta eventuali spazi */

    while (*ptr == ' ' || *ptr == '\t') ptr++;

    /* copia il primo carattere del simbolo */

    *tok++ = *ptr;

    /* a seconda del carattere decide il tipo di simbolo */

    switch(*ptr++)
    {
        case '\n':
        type = EOL; break;
        case '&':
        type = AMPERSAND; break;
        case ';':
        type = SEMICOLON; break;
        // Redirezione dello standard ouput su un file #4
        case '>':
        type = file_path(); break;
        default:
        type = ARG;
        /* copia gli altri caratteri del simbolo */
        while(inarg(*ptr))
        *tok++ = *ptr++;
    }

    /* aggiunge \0 al fondo */

    *tok++ = '\0';
    return(type);

}

int inarg(char c)   /* verifica se c non e' un carattere speciale */
{
    char *wrk;

    for (wrk = special; *wrk != '\0'; wrk++)
    if (c == *wrk) return(0);

    return(1);
}

void dellet_pathname()  /* elimina tutti i caratteri di pathname */
{
    int l = strlen(pathname);
    for (int i = 0; i <= l; i++)   
    {
        pathname[i] = '\0'; // pathname[i] = NULL
    }
}

int file_path() /* copia il percorso file in pathname, ritorna REDIRECT o APPEND */
{
    int i = 0;
    int l = strlen(inpbuf);
    int x = REDIRECT;
    dellet_pathname();  // strcpy(pathname, "");
    *tok++ = *ptr++;    // salta > in gettok
    // trova >
    while(inpbuf[i] != '>' && i <= l)
    {
        i++;
    }
    i++;
    // Redirezione dello standard ouput su un file in modalità APPEND #5
    if (inpbuf[i] == '>')
    {
        i++;
        *tok++ = *ptr++;    // salta > in gettok
        x = APPEND;
    }
    // salta gli spazzi
    while(inpbuf[i] == ' ' && i <= l)
    {
        i++;
    }
    // copia il percorso del file in pathname
    for (int j = 0; strchr(special, inpbuf[i]) == NULL && i <= l-1; j++)   
    {
        pathname[j] = inpbuf[i];
        i++;
        *tok++ = *ptr++;    // salta percorso file in gettok
    }
    return x;
}
