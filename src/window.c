//Simula una finestra
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "messages.h"
#include "utility.h"

//Variabile per la gestione degli errori
extern int errno;
/**
 * Flag globale per segnalare al device che il proprio parent deve comunicare con lui.
 * Viene modificato quando il device riceve un segnale SIGUSR1:
**/
int readParent = 0;

//Informazioni della window
Window window;

//Variabile per il calcolo di openTime
time_t start = -1;

//Handler eseguito alla ricezione di SIGUSR1
void handlerParent(int sig){
    readParent = 1;
}

/**
 * Flag globale per segnalare al device che il controllo manuale deve comunicare con lui.
 * Viene modificato quando il device riceve un segnale SIGUSR2:
**/
int readManual = 0;
//Handler eseguito alla ricezione di SIGUSR2
void handlerManual(int sig){
    readManual = 1;
}

/**
 * Gestisce il messaggio 'del' per window
**/
void delDevWindow(MessageDel del_msg, char fifo[]);

/**
 * Gestisce il messaggio 'info' per window
 * Risponde al padre con le informazioni del device o un ack negativo
**/
void infoDevWindow(MessageInfo info_msg, char fifo[]);

/**
 * Gestisce il messaggio 'check' per fridge
 * Risponde al padre con le proprie informazioni
**/
void checkDevWindow(MessageCheck check_msg, char fifo[]);

/**
 * Gestisce il messaggio 'list' per window
**/
void listDevWindow(MessageList list_msg, char fifo[]); 

/**
 * Gestisce il messaggio 'switch' per window
**/
void switchDevWindow(MessageSwitch switch_msg, char fifo[]);
/**
 * Gestisce il messaggio 'link' per window
**/
void linkDevWindow(MessageLink link_msg, char fifo[]);
/**
 * Gestisce il messaggio 'update' per window
**/
void updateDevWindow(MessageUpdate update_msg, char fifo[]);

int main(int argc, char* argv[]) {
    char msg[BUFFSIZE];
    char fifo[20]; 
    int fd;

    /**
     * Due possibili configurazioni degli argomenti:
     * 1. Id del dispositivo
     *    In questo caso, agli altri campi è assegnato un valore default.
     * 2. Id, stato, switch di chiusura, switch di apertura, tempo di apertura
     * In entrambi i casi, il nome è costruito a partire dall'id
    **/
    if(argc == 2) { //argv = [argc, id]
        sscanf(argv[1], "%d", &window.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(window.name, "Window_%03d", window.id);     //Genero il nome del device
        window.state=0;      //Di default la finestra è chiusa
        window.openSwitch = 0;
        window.closeSwitch = 0;
        window.openTime = 0;
    }   
    else if(argc == 6) {// argv = [argc, id, state, openSwitch, closeSwitch, openTime]
        sscanf(argv[1], "%d", &window.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(window.name, "Window_%03d", window.id);     //Genero il nome del device
        sscanf(argv[2], "%d", &window.state);	     //Copio lo state
        sscanf(argv[3], "%d", &window.openSwitch);   //Copio l'openSwitch
        sscanf(argv[4], "%d", &window.closeSwitch);  //Copio il closeSwitch
        sscanf(argv[5], "%ld", &window.openTime);	 //Copio openTime
        //Se la window è gia accesa inizio a contare la nuova frazione di openTime da adesso
        if(window.state)
            start = time(NULL);
    }
    else {  //Errore negli argomenti
        fprintf(stderr,"%d: Errore nella creazione del dispositivo\n",getpid());
        exit(-1);
    }
    
    //Assegna alla ricezione di SIGUSR1 e SIGUSR2 i handler precedentemente dichiarati
    signal(SIGUSR1,handlerParent);
    signal(SIGUSR2,handlerManual);

    while(1) {
        usleep(500000);
        //si mette in pausa se è chiusa
        if(!window.state && !readParent && !readManual){
            pause();
        }

        if(readParent){
            readParent=0;

            fd = open(fifo, O_RDONLY);
            if(fd == -1){
                fprintf(stderr,"%d: Errore di comunicazione con il dispositivo controllore: %s\n",getpid(), strerror(errno)); //Errore con il percorso della pipe
                exit(-1);
            }
            read(fd,msg,BUFFSIZE);
            
            close(fd);

            if(!strncmp(msg,"del", 3)){
                MessageDel del_msg=deSerializeDel(msg);
                delDevWindow(del_msg, fifo);
            } else if(!strncmp(msg,"info", 4)){
                MessageInfo info_msg = deSerializeInfo(msg);
                infoDevWindow(info_msg, fifo);
            } else if(!strncmp(msg, "list", 4)){
                MessageList list_msg = deSerializeList(msg);
                listDevWindow(list_msg, fifo);
            } else if(!strncmp(msg, "switch", 6)){
                MessageSwitch switch_msg = deSerializeSwitch(msg);
                switchDevWindow(switch_msg, fifo);
            } else if(!strncmp(msg, "check", 5)){
                MessageCheck check_msg = deSerializeCheck(msg);
                checkDevWindow(check_msg, fifo);
            }else if(!strncmp(msg, "link", 4)){
                MessageLink link_msg = deSerializeLink(msg);
                linkDevWindow(link_msg, fifo);
            }else if(!strncmp(msg, "update", 6)){
                MessageUpdate update_msg = deSerializeUpdate(msg);
                updateDevWindow(update_msg, fifo);
            }
        }//Gestione del controllo manuale
        if(readManual){
            readManual=0;
            
            fd = open(MANUAL_FIFO, O_RDONLY);
            if(fd == -1){
                fprintf(stderr,"%d: Errore di comunicazione con il controllo manuale: %s\n",getpid(), strerror(errno)); //Errore con il percorso della pipe
                exit(-1);
            }
            read(fd, msg, BUFFSIZE);
            close(fd);

            if(!strncmp(msg,"switch", 6)){
                MessageSwitch switch_msg = deSerializeSwitch(msg); //assumo l'ack che segna che arriva dal controllo manuale già settato
                switchDevWindow(switch_msg, MANUAL_FIFO);
            }
        }
    }
}

void delDevWindow(MessageDel del_msg, char fifo[]) {
    int ack,id; //variabili per un nuovo eventuale messaggio
    int se_eliminare; //0=non terminerà, 1=terminerà alla fine
    if(window.id==del_msg.id || del_msg.id==0){ //trovato l'id del device
        if(del_msg.save==1){
            //salvo il dispositivo su file
            char file[20];
            sprintf(file,"%s%d.txt", PATH_FILE, window.id);
            saveWindow(file, window);
        }
        //il processo terminerà e se serve invierà ack positivo
        ack=1;
        se_eliminare=1;
    }else{
        //il processo non terminerà e manderà ack negativo
        ack=2;
        se_eliminare=0;
        
    }
    //se updateParent=1 devo mandare un messaggio di risposta al padre
    if(del_msg.updateParent){
        id=getpid();
        MessageDel msg= createDel(id, 0); //creo il messaggio di delete
        msg.updateParent=se_eliminare;//se dovrà essere eliminato, il padre dovrà aggiornare la lista dei figli
        msg.ack=ack;
        msg.id=id;
        
        char serialised_msg[BUFFSIZE]; //dim migliore?
        serializeDel(serialised_msg,msg);//trasformo il messaggio in stringa
        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);//mando il messaggio al padre
        close(fd);
    }else{
        if(ack==2){
            //Non dovrebbe mai succedere (Non si elimina ma il padre si??)
            fprintf(stderr,"%d: Errore nell'eliminazione del dispositivo.\n",getpid());
        }
    }
    if(se_eliminare){
        exit(0);
    }
}

void infoDevWindow(MessageInfo info_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];

    if(window.id==info_msg.id || info_msg.id == 0){
        //Trovato l'id del device
        info_msg.ack = 1;           //Setto ack positivo
        info_msg.type = WINDOW;     //Indico il tipo delle informazioni

        //Aggiorno il valore di openTime
        if(window.state) {
            window.openTime +=  time(NULL) - start;
            start = time(NULL);
        }

        //Incapsulo all'interno del messaggio le info del device
        serializeWindow(info_msg.device, window);
    }else{
        //Rimando il messaggio al padre con ack negativo
        info_msg.ack = 2;
    }
    serializeInfo(serialised_msg, info_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

void checkDevWindow(MessageCheck check_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    if(window.id == check_msg.id || check_msg.id == 0) {
        //Trovato l'id cercato
        check_msg.ack = 1;
        check_msg.id = window.id;
        check_msg.pid = getpid();
        check_msg.type = WINDOW;
        check_msg.isControl = 0;
        //genero il messaggio stringa
        serializeCheck(serialised_msg, check_msg);
        //invio il messaggio
        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);
        close(fd);
    } else {
        check_msg.ack = 2;
        serializeCheck(serialised_msg, check_msg);
        //invio il messaggio
        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);
        close(fd);
    }
}

void listDevWindow(MessageList list_msg, char fifo[]){
    //Dato il livello n, lascia n spazi prima di stamparsi
    int i;
    for(i=0;i<list_msg.level;i++){
        printf("\t");
    }
    char state[10];
    if(window.state){
        strcpy(state,"OPEN");
    }else{
        strcpy(state,"CLOSED");
    }
    printf("| -%s  %s\n",window.name,state);

    //risponde al padre
    list_msg.ack=1;
    char serialised_msg[BUFFSIZE];
    serializeList(serialised_msg, list_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE );
    close(fd);
}

void switchDevWindow(MessageSwitch switch_msg, char fifo[]) {
    int initialState = window.state;
    char serialised_msg[BUFFSIZE];
    switch_msg.ack = 3;     //Di default l'ack ritornato al padre è negativo (id invalido / irraggiungibile)
    
    //Se il label indica 'allSwitch' deve essere tradotto in due possibili switch (open e close)
    if(!strncmp(switch_msg.label, "allSwitch", 9)) {
        if(!strncmp(switch_msg.pos, "on", 2)) {
            strcpy(switch_msg.label, "open");
        }
        else if(!strncmp(switch_msg.pos, "off", 3)) {
            strcpy(switch_msg.label, "close");
            strcpy(switch_msg.pos, "on");
        }
    }

    if(window.id==switch_msg.id || switch_msg.id == 0){        
        //Controlla se il tipo di label è valido per questo device
        if(!strncmp("open", switch_msg.label, 4)) {
            if(!strncmp("on", switch_msg.pos, 2)) {
                window.openSwitch = 1;
                window.state = 1;
                //La posizione dell'interruttore torna subito a 'off'
                window.openSwitch = 0;
                //L'ack viene settato a positivo (cambio di stato)
                switch_msg.ack = 1;
                //Viene indicato il nuovo stato del device
                switch_msg.newState = window.state;
            }else if(!strncmp("off", switch_msg.pos, 3)) {
                //L'ack viene settato a positivo (cambio di stato)
                switch_msg.ack = 1;
                //Viene indicato lo stato del device
                switch_msg.newState = window.state;
            }else {
                //L'ack viene settato a negativo (pos invalida)
                switch_msg.ack = 5;
            }
        }else if(!strncmp("close", switch_msg.label, 5)) {
            if(!strncmp("on", switch_msg.pos, 2)) {
                window.closeSwitch = 1;
                window.state = 0;
                //La posizione dell'interruttore torna subito a 'off'
                window.closeSwitch = 0;
                //L'ack viene settato a positivo (cambio di stato)
                switch_msg.ack = 1;
                //Viene indicato il nuovo stato del device
                switch_msg.newState = window.state;
            }else if(!strncmp("off", switch_msg.pos, 3)) {
                //L'ack viene settato a positivo (cambio di stato)
                switch_msg.ack = 1;
                //Viene indicato lo stato del device
                switch_msg.newState = window.state;
            }else {
                //L'ack viene settato a negativo (pos invalida)
                switch_msg.ack = 5;
            }
        }else {
            //L'ack viene settato a negativo (label invalido)
            switch_msg.ack = 4;
        }
    }

    //Se la window cambia stato aggiorno il suo openTime
    if(switch_msg.ack == 1) {
        if(switch_msg.newState == 1) {
            start = time(NULL);
        }
        else if(switch_msg.newState == 0 && initialState == 1) {
            window.openTime += time(NULL) - start;
            start = -1;
        }
    }

    //Invia al padre il messaggio
    serializeSwitch(serialised_msg, switch_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}


void linkDevWindow(MessageLink link_msg, char fifo[]) {
    //rispondo con ack negativo
    char serialised_msg[BUFFSIZE];
    link_msg.ack=2;
    serializeLink(serialised_msg, link_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

void updateDevWindow(MessageUpdate update_msg, char fifo[]){
    //rispondo con type 4(bulb)
    char serialised_msg[BUFFSIZE];
    update_msg.ack=1;
    update_msg.type=4;
    serializeUpdate(serialised_msg,update_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}