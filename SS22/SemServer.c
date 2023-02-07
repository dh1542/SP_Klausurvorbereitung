#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#define MAX_LINE_LEN 20

struct client {
    FILE *rx, *tx;
    SEM *requests;
};
struct request {
    struct client *client;
    char line[];
};

// vorgegebene Funktionen:
void die(const char *name) {
    perror(name);
    exit(EXIT_FAILURE);
}
// parse the string @str as number or pointer into *@dst.
// returns 0 on success or in case of an error -1 and sets errno.
int parseNumber(const char *str, int *dst);
int parsePointer(const char *str, void **dst);

// send @fmt to @f as reply to request @line
void reply(FILE *f, const char *line, const char *fmt, ...);

struct list;

// create and return a new list or NULL; sets errno in case of an error
struct list *listCreate(void);

// add @sem to @list or return -1; sets errno in case of an error
int listAppend(struct list *list, SEM *sem);

// returns 1 if @list contains @sem, 0 otherwise
int listContains(const struct list *list, SEM *sem);
typedef struct SEM SEM;

// create and return a new semaphore or NULL and set errno in case of an error
SEM *semCreate(int initVal);

// destroy a semaphore and free all associated resources.
void semDestroy(SEM *sem);

// P- and V-operations never fail.
void P(SEM *sem);
void V(SEM *sem);

// Global variables
static struct list *list;

/* Initialisiert den globalen Zustand des Prozesses, nimmt
Verbindungen an und erzeugt zu jeder eine struct client (clientCreate()). Die wei-
tere Bearbeitung erfolgt in einem eigenen Thread, der über die Funktion workerStart() ge-
startet wird. Dieser werden die Bearbeitungsfunktion clientProcess() sowie die client
-Struktur übergeben. Fehler werden auf stderr ausgegeben, falls möglich wird die Ausfüh-
rung des Prozesses aber fortgesetzt. */
int main(int argc, char *argv){
    // create list
    list = listCreate();
    if(list == NULL){die("listCreate");}

    // create server -> socket, struct sockaddr, bind, listen, wait and accept

    // socket
    int ls = socket(AF_INET6, SOCK_STREAM, 0);
    if(ls < 0){die("socket");}

    // struct
    struct sockaddr_in6 *addr = {
        .sin6_family = AF_INET6;
        .sin6_port = htons(7076);
        .sin6_addr = in6addr_any;
    };

    // bind
    if(bind(ls, struct sockaddr &addr, sizeof(addr)) < 0){die("bind");}

    // listen
    if(listen(ls, SOMAXCONN) < 0){die("listen");}

    // ignore sigpip
    struct sigaction action = {
        .sa_handler = SIG_IGN;
    }

    if(sigemptyset(&action.sa_flags) < 0){die("sigemptyset");}
    if(sigaction(SIGPIPE, &action, NULL) < 0){die("sigaction");}



    // wait for connections and accept them
    // create client for each 
    while(1){

        // client socket
        int cs = accept(ls, NULL, NULL);
        if(cs < 0){perror("accept"); continue;}

        struct client *client = clientCreate(cs);
        if(client == NULL){perror("clientCreate"); continue;}

        if(workerStart(clientProcess, client) < 0){perror("workerStart"); clientDestroy(client);}

        
    }


}

/* Startet fn mit arg als Argument in
einem neuen Faden, dessen Ressourcen automatisch bei dessen Beendigung freigegeben wer-
den. Im Fehlerfall wird errno auf einen passend Wert gesetzt und -1 statt 0 zurückgegeben. */
int workerStart(void *(*fn)(void *), void *arg){
    // create new thread -> pthread create, detach
    pthread_t thread;


    errno = 0;
    // set errno
    errno = pthread_create(&thread, NULL, fn, arg);
    
    // if errno was set, return -1
    if(errno != 0){
        return -1;
    }

    errno = pthread_detach(thread);
    if(errno != 0){
        return -1;
    }

    return 0;
}

/* Erzeugt eine struct client für die Verbin-
dung fd. Im Fehlerfall wird errno gesetzt, NULL zurückgegeben und belegte Ressourcen
freigegeben. In jedem Fall darf fd vom Aufrufer nicht weiter verwendet werden. */
struct client *clientCreate(int fd){
    
    // create client struct
    struct client client = malloc(sizeof(*client));
    if(client == NULL){
        close(fd);
        return NULL;
    }

    // SEM Struct
    struct SEM SEM = semCreate(0);
    if(SEM == NULL){
        close(fd);
        free(client)l=;
        return NULL;
    }

    client -> requests = SEM;

    // file descriptors
    client -> rx = fdopen(fd, "r");
    if(client -> rx == NULL){
        close(fd);
        free(client);
        return NULL;
    }

    // duplicate for tx
    int fd2 = dup(fd);
    if(fd2 == -1){
        fclose(client -> rx);
        close(fd2);
        close(fd);
        free(client);
        return NULL;
    }
    
    client -> rx = fdopen(fd2, "w");
    if(client -> rx == NULL){
        fclose(client -> rx);
        close(fd);
        close(fd2);
        free(client);
        return NULL;
    }

    return client;

}

/* Liest Zeilen mit Arbeitsaufträgen von der in arg als
struct client übergebenen Verbindung ein und startet Fäden zu deren Bearbeitung.
Zeilen mit mehr als 20 (MAX_LINE_LEN) Zeichen werden ohne Meldung verworfen. Für
alle anderen wird eine struct request erzeugt un in einem neuen Faden durch die
Funktion handleRequest() bearbeitet. Treten dabei Fehler auf, wird der Fehlergrund
(errno) mit der Funktion reply() an den Clienten geschickt (strerror(3)). Wurden alle
Zeilen eingelesen, muss mit dem Aufräumen der struct client gewartet werden, bis alle
Anfragen bearbeitet wurden, wozu der Semaphor requests in der Struktur vorgesehen ist. */
void *clientProcess(void *arg){

    // read with fgets()
    struct client *client = arg;

    // buffer (+2 bc \n \0)
    char buffer[MAX_LINE_LEN + 2];

    // counter for requests
    size_t request_counter = 0;

    while(fgets(buffer, sizeof(buffer), client -> rx)){
        size_t len = sizeof(buffer);

        // line length
        if(len > MAX_LINE_LEN && buffer[len - 1] != '\n'){
            // zeile zu lang, muss aber ausgelesen werden
            // mit fgetc, da wir nicht wissen wielange es noch geht
            while(fgetc(client -> rx) != EOF){
                if(c == '\n') break;
            }

            // check errors of stream with ferror
            if(ferror(client -> rx) != 0){
                reply(client -> tx, buffer, "%s", strerror(errno));
            }
            continue;
        }
        
        // length is ok, we can continue
        struct request *request = malloc(sizeof(*request));
        if(request == NULL){
            reply(client -> tx, buffer, "%s", strerror(errno));
            continue;
        }

        // copy string to request struct
        buffer[len - 1] = '\0';
        strcpy(request -> line, buffer);
        request -> client = client;
        
        // start worker
        if(workerStart(handleRequest, request) < 0){
            reply(client -> tx, buffer, "%s", strerror(errno));
            free(request);
            continue;
        }
        else{
            request_counter++;
        }
    }

    while(request_counter > 0){
        P(client -> requests);
        requests--;
    }
    // destroy client
    clientDestroy(client);
    return NULL;
}


/* Gibt die Ressourcen von client frei. */
void clientDestroy(struct client *client){
    // call sem destroy
    semDestroy(client -> SEM);
    
    // close file streams
    fclose(client -> rx);
    fclose(client -> tx);

    // free memory
    free(client);
}

/* Bearbeitet das als arg übergebene struct request-
Objekt. Abhängig vom ersten Zeichen in line wird handleI() oder handlePV() aus-
geführt. Geben diese einen Wert ungleich 0 zurück, wird der Fehlergrund (errno) als
Antwort verwendet. Andernfalls haben die Funktion selbst bereits eine Antwort geschickt.
Auf ungültige Operationen wird mit "unknown operation" geantwortet (reply()).*/
void *handleRequest(void *arg){
    // request struct
    struct request *request = arg;
    
    int number = 0;
    // get first char in line
    char c = (request -> line)[0];

    if(c == 'P'){
        number = handlePV(request)
    }
    else if(c == 'I'){
        number = handleI(request);
    }
    else if(c == 'V'){
        handlePV(request)

    }
    else{
        // error -> reply
        reply(request -> client -> tx, request -> line, "unknown_operation");
    }

    if(numer != 0){
        reply(request -> client -> tx, request -> line, "%s", strerror(errno));
    }

    V(request -> client -> requests);
    free(request);
    return NULL;
}

/*  Initialisiert einen neuen Semaphor mit dem
in der Anfragezeile angegebenen Initialwert (parseNumber()). Im Erfolgsfall wird die
Speicherdresse des Semaphors in die Liste der gültigen Semaphore eingetragen (listAdd())
und mit reply() als Antwort geschickt.*/
int handleI(const struct request *rq){

    int val;

    // intial value of sem 
    if(parseNumber(rq -> line[1], &val) != 0){
        return -1;
    }

    // create SEM
    SEM *sem = semCreate(val);
    if(sem == NULL){
        return -1;
    }

    if(listAppend(&semaphore, sem) != 0){
        semDestroy(sem);
        return -1;
    }

    reply(rq -> client -> tx, rq -> line, "%p", sem);
    return 0;
}

/*  Prüft, ob die in der
Anfragezeile gegebene Adresse (parsePointer()) gültig ist (listContains()) und
führt die Funktion fn darauf aus. Für ungültige Adressen wird errno=ENOENT gesetzt und
-1 zurückgegeben, andernfalls "success" als Antwort (reply()) gesendet.*/
int handlePV(const struct request *rq, void (*fn)(SEM *)){
    void *addr;
    if(parsePointer(rq -> line[1], &addr) != 0){
        return -1;
    }

    if(!(listContains(&Semaphors, addr))){
        errno = ENOENT;
        return -1;
    }

    fn(addr);
    reply(rq -> client -> tx, rq -> line, "success");
    return 0;
}









