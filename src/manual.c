#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "utility.h"
#include "messages.h"

int controller_pid;

void switchDev(int id, char* label, char* pos);

int main(int argc, char **argv) {

    char cmd[BUFFSIZE];
	char args[MAXARGS][BUFFSIZE];
    char delimiter[2] = " ";
    
    //Legge da file il pid della centrallina
    
    FILE *stream_pid = fopen(CONTROLLER_TXT, "r");
    //controlla se c'è il file
    if(!stream_pid){
        printf("Centralina non in esecuzione\n");
        return -1;
    }
    
    fscanf(stream_pid, "%d", &controller_pid);
    fclose(stream_pid);
    //printf("%d\n",controller_pid);

    //controlla se la centralina è in esecuzione
    int sig_error = kill(controller_pid, 0);
    if(sig_error == -1){
        printf("Centralina non in esecuzione\n");
        exit (-1);
    }
    //Crea la fifo di comunicazione
    mkfifo(MANUAL_FIFO, 0666);

    printf("\t\tCONTROLLO MANUALE\n");

    int i;
    while(1) {

        //Aspetta un input dall'utente
        printf("> ");
		fgets(cmd, BUFFSIZE, stdin);
        strtok(cmd, "\n");

        //Divide il comando in segmenti delimitati da uno spazio
		char* arg = strtok (cmd, delimiter);
        for(i=0; i<MAXARGS && arg!=NULL; i++) {
			strcpy(args[i], arg);
			arg = strtok (NULL, delimiter);
		}

        //Controllo del tipo di comando
        if(!strcmp(args[0], "quit") || !strcmp(args[0], "q")) {

            break;

        }  else if(!strcmp(args[0], "switch")) {

            switchDev(strtoint(args[1]), args[2], args[3]);
            //printf("SWITCHING ID %s LABEL %s to %s\n",args[1], args[2], args[3]);

        }  else {

            printRed("ERRORE: comando non riconosciuto.\n");

        }
    }
    return 0;
}


void switchDev(int id, char* label, char* pos) {

    if(id<=0){
        printf("ID non valido\n");
        return;
    }

	MessageCheck msg_check = createCheck(id,0);
    char serialised_msg[BUFFSIZE];
    serializeCheck(serialised_msg,msg_check);
    //controlla se il processo della centralina è in esecuzione
    int sig_error= kill(controller_pid,SIGUSR2);
    if(sig_error==-1){
        printf("Centralina non in esecuzione\n");
        exit(-1);
    }
    int mfd = open(MANUAL_FIFO, O_WRONLY);
    if(!mfd){
        printf("Errore fifo comando manuale\n");
        exit(-1);
    }
    write(mfd,serialised_msg,BUFFSIZE);
    close(mfd);

    mfd=open(MANUAL_FIFO, O_RDONLY);
    read(mfd,serialised_msg,BUFFSIZE);
    close(mfd);
    msg_check=deSerializeCheck(serialised_msg);
    
    if(msg_check.ack==2){
        printf("Nessun dispositivo con ID %d collegato\n",id);
        return;
    }

    int pidDevice = msg_check.pid;

    //Creo il messaggio di tipo switch
    MessageSwitch msg_switch = createSwitch(id, label, pos);
    serializeSwitch(serialised_msg, msg_switch); //Trasformo il messaggio in stringa   
    //Manda il messaggio direttamente al dispositivo interessato tramite la fifo dedicata
    kill(pidDevice, SIGUSR2);
    mfd = open(MANUAL_FIFO, O_WRONLY);
    write(mfd,serialised_msg,BUFFSIZE);
    close(mfd);
    //Leggo la risposta di nuovo dalla fifo dedicata
    mfd=open(MANUAL_FIFO, O_RDONLY);
    read(mfd,serialised_msg,BUFFSIZE);
    close(mfd);

    msg_switch=deSerializeSwitch(serialised_msg);
    switch(msg_switch.ack) {
        case 1: printf("Switch avvenuto con successo.\n"); //cambio di stato
        break;
        case 2: printf("Switch avvenuto con successo.\n"); //nessun cambio di stato
        break;
        case 3: printf("Il device specificato non esiste o non è raggiungibile dal controller.\n");
        break;
        case 4: printf("Il label specificato non esiste nel device indicato.\n");
        break;
        case 5: printf("La posizione indicata dallo switch non è valida per il label specificato.\n");
        break;
        default: printf("Ack dal comando 'switch' non riconosciuto. (%d)\n", msg_switch.ack);
    }
    
}
