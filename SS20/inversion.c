/** 10 Punkte */


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


int handle(int fd){
    // Erstellen der FILE* Abstraktionen
    FILE *fileOUT = fdopen(fd, "w");
    if(!fileOUT){die("fdopen");}
    
    // duplicate fd  
    int fd2 = dup(fd);
    if(fd2 == -1){die("dup");}

    FILE *fileIN = fdopen(fd2, "r");
    if(!fileIN){die("fdopen");}
    

    // Einlesen der Eingabe

    // dynamically allocate to string

    size_t size = 0;
    char *string = malloc(1);
    if(!string){die("malloc");}
    

    // read character wise
    int cur;
    while((cur = fgetc(fileIN)) != EOF){
        // realloc string
        
        string = realloc(string, size++);
        if(!string){die("realloc");}

        string[size - 1] = (char) cur;
        
    }

    if(ferror(fileIN)){die("ferror");}
    if(fflush(fileIN)){die("fflush");}

    // Ausgabe der Daten
    for(int i = size - 1; i >= 0; i--){
        
        if(fputc(string[i], fileOUT) == EOF){
            fflush(fileOUT);
            die("fputc");
        }
    }

    if(fclose(fileIN) == EOF){die("fclose");}
    if(fclose(fileOUT) == EOF){die("fclose");}

    // don't forget to free
    free(string);


    return 0;
    
}


int main(){

    int fd = open("inversion_test.txt", O_RDONLY);
    if(!fd){
        die("open");
    }

    handle(fd);
}