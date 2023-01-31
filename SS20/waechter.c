#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#define MAX_CONTACT_TIME 60
#define MAX_CONTACTS 5
#define PORT 2020

typedef unsigned long time_t;
typedef struct contact {
    pid_t pid;
    time_t deadline;
} contact_t;


struct list { /* ... */ };

// @list initialisieren.
void list_init(struct list *list);

// Rückgabe: Erstes Element in @list oder NULL, falls leere Liste.
struct contact *list_peek(struct list *list);

// @item an das Ende von @list anfügen.
void list_enqueue(struct list *list, struct contact *item);

// Entfernt das erste Element aus @list.
// Rückgabe: Erstes Element in @list oder NULL, falls leere Liste.
struct contact *list_dequeue(struct list *list);

// Entfernt das erste Element von @list mit passender @pid.
// Rückgabe: entferntes Element oder NULL, falls kein Treffer.
struct contact *list_remove(struct list *list, pid_t pid);

// Behandlung einer angenommenen Verbindung.
int handle(int fd);

// Erzeugt eine Zeitstempel, der @seconds nach dem Aufrufzeitpunkt ist.
time_t make_timestamp(unsigned seconds);

// Setzt einen neuen Zeitstempel für SIGALRM. Gibt den zuletzt gesetzten
// Zeitstempel zurueck oder 0 falls inaktiv.
time_t set_deadline(time_t timestamp);

void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

static struct list running, killed, unused;





/* Sendet an Kinder, deren maximale Kontaktzeit abgelaufen ist
SIGKILL und verschiebt deren Verwaltungsstruktur in die Liste killed.*/
static void handle_signalrm(int sig){
    // iterates over list with running processes and checks wether timestamp is expired

    // block SIGALRM and SIGCHLD
    sigset_t signal_set;
    if(sigemptyset(&signal_set) < 0){die("sigemptyset");}
    if(sigaddset(%signal_set, SIGALRM) < 0){die("sigaddset");}
    if(sigaddset(%signal_set, SIGCHLD) < 0){die("sigaddset");}

    if(sigprocmask(SIG_BLOCK, &signal_set, NULL) < 0){die("sigprocmask");}

    struct list *running_list = &running;

    while(running_list){
        contact_t *head = list_peek(&running_list);

        // time stamp is ok
        if(head -> deadline <= make_timestamp(0)){
            continue;
        }

        // timestamp is not ok -> send SIGKILL
        if(kill(head -> pid, SIGKILL) < 0){
            perror("kill");
            continue;
        }

        // kick out of running list
        list_dequeue(&running_list);
        list_enqueue(&killed, head);

        running_list = running_list -> next;

    }

    if(sigprocmask(SIG_UNBLOCK, &signal_set, NULL) < 0){die("sigprocmask");}
}

