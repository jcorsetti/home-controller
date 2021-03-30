//Simula una lampadina
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

extern int errno;
/**
 * Flag globale per segnalare al device che il proprio parent deve comunicare con lui.
 * Viene modificato quando il device riceve un segnale SIGUSR1:
**/
int readParent = 0;

//Informazioni del bulb
Bulb bulb;

//Variabile per il calcolo di activeTime
time_t start = 0;

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
 * Gestisce il messaggio 'del' per bulb
**/
void delDevBulb(MessageDel del_msg, char fifo[]);

/**
 * Gestisce il messaggio 'info' per bulb
 * Risponde al padre con le informazioni del device o un ack negativo
**/
void infoDevBulb(MessageInfo info_msg, char fifo[]);

/**
 * Gestisce il messaggio 'check' per bulb
 * Risponde al padre con le proprie informazioni
**/
void checkDevBulb(MessageCheck check_msg, char fifo[]);

/**
 * Gestisce il messaggio list
**/
void listDevBulb(MessageList list_msg, char fifo[]); 

/**
 * Gestisce il messaggio 'switch' per bulb
**/
void switchDevBulb(MessageSwitch switch_msg, char fifo[]);
/**
 * Gestisce il messagio link
**/
void linkDevBulb(MessageLink link_msg, char fifo[]);

/**
 * Gestisce il messagio update
**/
void updateDevBulb(MessageUpdate update_msg, char fifo[]);

int main(int argc, char* argv[]) {
    
    char msg[BUFFSIZE];
    char fifo[20]; 
    int fd;

    /**
     * Due possibili configurazioni degli argomenti:
     * 1. Id del dispositivo
     *    In questo caso, agli altri campi è assegnato un valore default.
     * 2. Id, stato, interruttore e tempo di attivazione
     * In entrambi i casi, il nome è costruito a partire dall'id
    **/
    if(argc == 2) { // argv = [argc, id]
        sscanf(argv[1], "%d", &bulb.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(bulb.name, "Bulb_%03d", bulb.id);     //Genero il nome del device
        bulb.state = 0;      //Di defualt la lampadina è spenta
        bulb.mainSwitch = 0;
        bulb.activeTime = 0;
    }   
    else if(argc == 5) { // argv = [argc, id, state, switch, activeTime]
        sscanf(argv[1], "%d", &bulb.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(bulb.name, "Bulb_%03d", bulb.id);     //Genero il nome del device
        sscanf(argv[2], "%d", &bulb.state);
        sscanf(argv[3], "%d", &bulb.mainSwitch);
        sscanf(argv[4], "%ld", &bulb.activeTime);
        //Se il bulb è gia acceso inizio a contare la nuova frazione di activeTime da adesso
        if(bulb.state)
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
        //si mette in pausa se è spenta o se nessuno vuole comunicare
        if(!bulb.state && !readParent && !readManual){
            pause();
        }
        if(readParent) {
            readParent=0;

            fd = open(fifo, O_RDONLY);
            if(fd == -1){
                fprintf(stderr,"%d: Errore di comunicazione con il dispositivo controllore: %s\n",getpid(), strerror(errno)); //Errore con il percorso della pipe
                exit(-1);
            }
            read(fd, msg, BUFFSIZE);

            close(fd);

            if(!strncmp(msg, "del", 3)){
                MessageDel del_msg = deSerializeDel(msg);
                delDevBulb(del_msg, fifo);
            }else if(!strncmp(msg, "info", 4)){
                MessageInfo info_msg = deSerializeInfo(msg);
                infoDevBulb(info_msg, fifo);
            }else if(!strncmp(msg, "list", 4)){
                MessageList list_msg = deSerializeList(msg);
                listDevBulb(list_msg, fifo);
            }else if(!strncmp(msg, "switch", 6)){
                MessageSwitch switch_msg = deSerializeSwitch(msg);
                switchDevBulb(switch_msg, fifo);
            }else if(!strncmp(msg, "check", 5)){
                MessageCheck check_msg = deSerializeCheck(msg);
                checkDevBulb(check_msg, fifo);
            }else if(!strncmp(msg, "link", 4)){
                MessageLink link_msg = deSerializeLink(msg);
                linkDevBulb(link_msg, fifo);
            }else if(!strncmp(msg, "update", 6)){
                MessageUpdate update_msg = deSerializeUpdate(msg);
                updateDevBulb(update_msg, fifo);
            }
        }
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
                
                switchDevBulb(switch_msg, MANUAL_FIFO);
            }
        }
    }
}

void delDevBulb(MessageDel del_msg, char fifo[]) {
    int ack,id; //variabili per un nuovo eventuale messaggio
    int se_eliminare; //0=non terminerà, 1=terminerà alla fine
    if(bulb.id==del_msg.id || del_msg.id==0){ //trovato l'id del device
        if(del_msg.save==1){
            char file[20];
            sprintf(file,"%s%d.txt", PATH_FILE, bulb.id);
            saveBulb(file, bulb);
            //salvo il dispositivo su file
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
        MessageDel msg = createDel(id, 0); //creo il messaggio di delete
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

void infoDevBulb(MessageInfo info_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    
    if(bulb.id==info_msg.id || info_msg.id == 0){
        //Trovato l'id del device
        info_msg.ack = 1;           //Setto ack positivo
        info_msg.type = BULB;       //Indico il tipo delle informazioni

        //Aggiorno il valore di activeTime
        if(bulb.state) {
            bulb.activeTime +=  time(NULL) - start;
            start = time(NULL);
        }

        //Incapsulo all'interno del messaggio le info del device
        serializeBulb(info_msg.device, bulb);
    }else{
        //Rimando il messaggio al padre con ack negativo
        info_msg.ack = 2;
    }
    serializeInfo(serialised_msg, info_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

void checkDevBulb(MessageCheck check_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    if(bulb.id == check_msg.id || check_msg.id == 0) {
        //Trovato l'id cercato
        check_msg.ack = 1;
        check_msg.id = bulb.id;
        check_msg.pid = getpid();
        check_msg.type = BULB;
        check_msg.isControl = 0;
        check_msg.oldestParent = getpid();
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

void listDevBulb(MessageList list_msg, char fifo[]){
    //Dato il livello n, lascia n spazi prima di stamparsi
    int i;
    for(i=0;i<list_msg.level;i++){
        printf("\t");
    }
    char state[5];
    if(bulb.state){
        strcpy(state,"ON");
    }else{
        strcpy(state,"OFF");
    }
    printf("| -%s  %s\n",bulb.name,state);

    //risponde al padre (le info del messaggio non sono importani, basta che risponda qualcosa)
    list_msg.ack=1;
    char serialised_msg[BUFFSIZE];
    serializeList(serialised_msg, list_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE );
    close(fd);
}

void switchDevBulb(MessageSwitch switch_msg, char fifo[]) {
    int initialState = bulb.state;
    char serialised_msg[BUFFSIZE];
    switch_msg.ack = 3;     //Di default l'ack ritornato al padre è negativo (id invalido / irraggiungibile)
    
    //Se il label indica 'allSwitch' equivale a 'main' e quindi basta sostituirlo
    if(!strncmp(switch_msg.label, "allSwitch", 9)) {
        strcpy(switch_msg.label, "main");
    }

    if(bulb.id==switch_msg.id || switch_msg.id == 0){        
        //Controlla se il tipo di label è valido per questo device
        if(!strncmp("main", switch_msg.label, 4)) {
            if(!strncmp("on", switch_msg.pos, 2)) {
                bulb.mainSwitch = 1;
                bulb.state = 1;
                //L'ack viene settato a positivo (cambio di stato)
                switch_msg.ack = 1;
                //Viene indicato il nuovo stato del device
                switch_msg.newState = bulb.state;
            }else if(!strncmp("off", switch_msg.pos, 3)) {
                bulb.mainSwitch = 0;
                bulb.state = 0;
                //L'ack viene settato a positivo (cambio di stato)
                switch_msg.ack = 1;
                //Viene indicato il nuovo stato del device
                switch_msg.newState = bulb.state;
            }else {
                //L'ack viene settato a negativo (pos invalida)
                switch_msg.ack = 5;
            }
        }else {
            //L'ack viene settato a negativo (label invalido)
            switch_msg.ack = 4;
        }
    }

    //Se il bulb viene acceso aggiorno il suo activeTime
    if(switch_msg.ack == 1) {
        if(switch_msg.newState == 1) {
            start = time(NULL);
        }
        else if(switch_msg.newState == 0 && initialState == 1) {
            bulb.activeTime += time(NULL) - start;
            start = -1;
        }
    }

    //Invia al padre il messaggio
    serializeSwitch(serialised_msg, switch_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

void linkDevBulb(MessageLink link_msg, char fifo[]) {
    //rispondo con ack negativo
    char serialised_msg[BUFFSIZE];
    link_msg.ack=2;
    serializeLink(serialised_msg, link_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}


void updateDevBulb(MessageUpdate update_msg, char fifo[]){
    //rispondo con type 3(lampadina)
    char serialised_msg[BUFFSIZE];
    update_msg.ack=1;
    update_msg.type=3;
    serializeUpdate(serialised_msg,update_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}