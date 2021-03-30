//Simula un frigorifero
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

//Informazioni del fridge
Fridge fridge;

//Variabile per il calcolo di openTime
time_t start = -1;
//Variabile per la chiusura automatica del fridge
time_t startDelay = -1;

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
 * Gestisce il messaggio 'del' per fridge
**/
void delDevFridge(MessageDel del_msg, char fifo[]);

/**
 * Gestisce il messaggio 'info' per fridge
 * Risponde al padre con le informazioni del device o un ack negativo
**/
void infoDevFridge(MessageInfo info_msg, char fifo[]);

/**
 * Gestisce il messaggio 'check' per fridge
 * Risponde al padre con le proprie informazioni
**/
void checkDevFridge(MessageCheck check_msg, char fifo[]);

/**
 * Gestisce il messaggio list per fridge
**/
void listDevFridge(MessageList list_msg, char fifo[]);

/**
 * Gestisce il messaggio 'switch' per fridge
**/
void switchDevFridge(MessageSwitch switch_msg, char fifo[]);

/**
 * Gestisce il messaggio 'link' per fridge
**/
void linkDevFridge(MessageLink link_msg, char fifo[]);
/**
 * Gestisce il messaggio 'update' per fridge
**/
void updateDevFridge(MessageUpdate update_msg, char fifo[]);
int main(int argc, char* argv[]) {
    char msg[BUFFSIZE];
    char fifo[20]; 
    int fd;

    /**
     * Due possibili configurazioni degli argomenti:
     * 1. Id del dispositivo.
     *    In questo caso, agli altri campi è assegnato un valore default.
     * 2. Id, stato, switch di apertura, termostato, tempo di apertura, tempo di delay, percentuale di riempimento e temperatura.
     * In entrambi i casi, il nome è costruito a partire dall'id
    **/
    if(argc == 2) { //1° Configurazione
        sscanf(argv[1], "%d", &fridge.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(fridge.name, "Fridge_%03d", fridge.id);     //Genero il nome del device
        // I seguenti parametri sono di default (da rivedere)
        fridge.state = 0;   
        fridge.mainSwitch = 0;   
        fridge.thermostat = 0;   
        fridge.openTime = 0;     
        fridge.delay = 10;        
        fridge.perc = 0;         
        fridge.temp = 0;
    }   
    else if(argc == 9) { // argv= [ argc, id, state, switch, termostato, openTime, delay, perc, temp]
        sscanf(argv[1], "%d", &fridge.id);              //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());     //Genero il nome della fifo di comunicazione
        sprintf(fridge.name, "Fridge_%03d", fridge.id);  //Genero il nome del device
        sscanf(argv[2], "%d", &fridge.state);
        sscanf(argv[3], "%d", &fridge.mainSwitch);
        sscanf(argv[4], "%d", &fridge.thermostat);
        sscanf(argv[5], "%ld", &fridge.openTime);
        sscanf(argv[6], "%ld", &fridge.delay);
        sscanf(argv[7], "%d", &fridge.perc);
        sscanf(argv[8], "%d", &fridge.temp);
        //Se il fridge è gia acceso inizio a contare la nuova frazione di openTime da adesso
        if(fridge.state)
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
        //si mette in pausa se è chiuso
        if(!fridge.state && !readParent && !readManual){
            pause();
        }
        if(readParent){
            
            readParent=0;
            fd = open(fifo, O_RDONLY);
            if(fd == -1){
                fprintf(stderr,"%d: Errore di comunicazione con il dispositivo controllore: %s\n",getpid(), strerror(errno)); //Errore con il percorso della pipe
                exit(-1);
            }
            read(fd, msg, BUFFSIZE);
            close(fd);
            //gestione dei messaggi
            if(!strncmp(msg,"del", 3)){
                MessageDel del_msg = deSerializeDel(msg);
                delDevFridge(del_msg, fifo);
            }else if(!strncmp(msg,"info", 4)){
                MessageInfo info_msg = deSerializeInfo(msg);
                infoDevFridge(info_msg, fifo);
            }else if(!strncmp(msg, "list", 4)){
                MessageList list_msg = deSerializeList(msg);
                listDevFridge(list_msg,fifo);
            }else if(!strncmp(msg, "switch", 6)){
                MessageSwitch switch_msg = deSerializeSwitch(msg);
                switchDevFridge(switch_msg, fifo);
            }else if(!strncmp(msg, "check", 5)){
                MessageCheck check_msg = deSerializeCheck(msg);
                checkDevFridge(check_msg, fifo);
            }else if(!strncmp(msg, "link", 4)){
                MessageLink link_msg = deSerializeLink(msg);
                linkDevFridge(link_msg, fifo);
            }else if(!strncmp(msg, "update", 6)){
                MessageUpdate update_msg = deSerializeUpdate(msg);
                updateDevFridge(update_msg, fifo);
            }
        }
        //Lettua della pipe per il controllo manuale
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
                switchDevFridge(switch_msg, MANUAL_FIFO);
            }
        }
        //Se il fridge è aperto controllo se è necessario chiuderlo automaticamente
        if(fridge.state) {
            if(time(NULL) >= startDelay + fridge.delay) {
                fridge.mainSwitch = 0;
                fridge.state = 0;
                startDelay = -1;
                fridge.openTime += time(NULL) - start;
                start = -1;
            }
        }
    }
}

void delDevFridge(MessageDel del_msg, char fifo[]) {
    int ack,id; //variabili per un nuovo eventuale messaggio
    int se_eliminare; //0=non terminerà, 1=terminerà alla fine
    if(fridge.id==del_msg.id || del_msg.id==0){ //trovato l'id del device
        if(del_msg.save==1){
            char file[20];
            sprintf(file,"%s%d.txt", PATH_FILE, fridge.id);
            saveFridge(file, fridge);
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
        MessageDel msg= createDel(id, 0); //creo il messaggio di delete
        msg.updateParent=se_eliminare;//se dovrà essere eliminato, il padre dovrà aggiornare la lista dei figli
        msg.ack=ack;
        msg.id=id;
        
        char serialised_msg[BUFFSIZE]; 
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

void infoDevFridge(MessageInfo info_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];

    if(fridge.id==info_msg.id || info_msg.id == 0){
        //Trovato l'id del device
        info_msg.ack = 1;           //Setto ack positivo
        info_msg.type = FRIDGE;     //Indico il tipo delle informazioni

        //Aggiorno il valore di openTime
        if(fridge.state) {
            fridge.openTime +=  time(NULL) - start;
            start = time(NULL);
        }

        //Incapsulo all'interno del messaggio le info del device
        serializeFridge(info_msg.device, fridge);
    }else{
        //Rimando il messaggio al padre con ack negativo
        info_msg.ack = 2;
    }
    serializeInfo(serialised_msg, info_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

void checkDevFridge(MessageCheck check_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    if(fridge.id == check_msg.id || check_msg.id == 0) {
        //Trovato l'id cercato
        check_msg.ack = 1;
        check_msg.id = fridge.id;
        check_msg.pid = getpid();
        check_msg.type = FRIDGE;
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

void listDevFridge(MessageList list_msg, char fifo[]){
    //Dato il livello n, lascia n spazi prima di stamparsi
    int i;
    for(i=0;i<list_msg.level;i++){
        printf("\t");
    }
    char state[10];
    if(fridge.state){
        strcpy(state,"OPEN");
    }else{
        strcpy(state,"CLOSED");
    }
    printf("| -%s  %s\n",fridge.name,state);

    //risponde al padre
    list_msg.ack=1;
    char serialised_msg[BUFFSIZE];
    serializeList(serialised_msg, list_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE );
    close(fd);
}

void switchDevFridge(MessageSwitch switch_msg, char fifo[]) {
    int initialState = fridge.state;
    char serialised_msg[BUFFSIZE];
    switch_msg.ack = 3;     //Di default l'ack ritornato al padre è negativo (id invalido / irraggiungibile)
    
    //Se il label indica 'allSwitch' equivale a 'main' e quindi basta sostituirlo
    if(!strncmp(switch_msg.label, "allSwitch", 9)) {
        strcpy(switch_msg.label, "main");
    }

    if(fridge.id==switch_msg.id || switch_msg.id == 0){       
        //Controllo se il tipo di label è valido per questo device
        if(!strncmp("main", switch_msg.label, 4)) {
            if(!strncmp("on", switch_msg.pos, 2)) {
                fridge.mainSwitch = 1;
                fridge.state = 1;
                //L'ack viene settato a positivo (cambio di stato)
                switch_msg.ack = 1;
                //Viene indicato il nuovo stato del device
                switch_msg.newState = fridge.state;
            }else if(!strncmp("off", switch_msg.pos, 3)) {
                    fridge.mainSwitch = 0;
                    fridge.state = 0;
                    //L'ack viene settato a positivo (cambio di stato)
                    switch_msg.ack = 1;
                    //Viene indicato il nuovo stato del device
                    switch_msg.newState = fridge.state;
            }else {
                //L'ack viene settato a negativo (pos invalida)
                switch_msg.ack = 5;
            }
        }else if(!strncmp("thermostat", switch_msg.label, 10)) {
            int newTemp;
            if(sscanf(switch_msg.pos, "%d", &newTemp)) {
                if(newTemp>=-15 && newTemp<=15){
                    fridge.thermostat = newTemp;
                    fridge.temp = newTemp;
                    //L'ack viene settato a positivo (nessun cambio di stato)
                    switch_msg.ack = 2;
                }else{
                    switch_msg.ack = 5;
                }
            }else {
                //L'ack viene settato a negativo (pos invalida)
                switch_msg.ack = 5;
            }
        }else if(!strncmp("perc", switch_msg.label, 10)) {
            int newPerc;
            if(sscanf(switch_msg.pos, "%d", &newPerc)) {
                if(newPerc>=0 && newPerc<=100){
                    fridge.perc = newPerc;
                    //L'ack viene settato a positivo (nessun cambio di stato)
                    switch_msg.ack = 2;
                }else{
                    switch_msg.ack = 5;
                }
            }else {
                //L'ack viene settato a negativo (pos invalida)
                switch_msg.ack = 5;
            }
        }
        else if(!strncmp("delay", switch_msg.label, 5)) {
            int newDelay = -1;
            if(sscanf(switch_msg.pos, "%d", &newDelay)) {
                if(newDelay >= 0) {
                    fridge.delay = newDelay;
                    //L'ack viene settato a positivo (nessun cambio di stato)
                    switch_msg.ack = 2;
                }else {
                    //L'ack viene settato a negativo (pos invalida)
                    //Delay non può essere negativo
                    switch_msg.ack = 5;
                }
            }else {
                //L'ack viene settato a negativo (pos invalida)
                switch_msg.ack = 5;
            }
        }else {
            //L'ack viene settato a negativo (label invalido)
            switch_msg.ack = 4;
        }
    }
    //Se il fridge cambia stato aggiorno il suo activeTime
    if(switch_msg.ack == 1) {
        if(switch_msg.newState == 1) {
            start = time(NULL);
            startDelay = time(NULL);
        }else if(switch_msg.newState == 0 && initialState == 1) {
            fridge.openTime += time(NULL) - start;
            start = -1;
            startDelay = -1;
        }
    }

    //Invia al padre il messaggio
    serializeSwitch(serialised_msg, switch_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}
void linkDevFridge(MessageLink link_msg, char fifo[]) {
    //rispondo con ack negativo
    char serialised_msg[BUFFSIZE];
    link_msg.ack=2;
    serializeLink(serialised_msg, link_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

void updateDevFridge(MessageUpdate update_msg, char fifo[]){
    //rispondo con type 5(fridge)
    char serialised_msg[BUFFSIZE];
    update_msg.ack=1;
    update_msg.type=5;
    serializeUpdate(serialised_msg,update_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}