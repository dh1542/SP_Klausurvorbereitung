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