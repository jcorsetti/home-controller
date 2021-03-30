#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "devList.h"
#include "devices.h"
#include "utility.h"
extern int errno;

/**Salva l'hub nel file specificato
 * file => nome del file di salvataggio
 * dev  => struct con le informazioni sull'hub
 * id   => lista degli id dei figli, memorizzati nello stesso ordine dei subDev in dev.list, o NULL es l'hub non ha figli 
 */
void saveHub(const char file[], const Hub dev, const int id[]) {
    FILE *fp = fopen(file,"w");
    if(!fp) {   //Controllo sull'apertura del file
        fprintf(stderr,"%d: errore apertura file: %s.",getpid(),strerror(errno));
        return;
    }
    fprintf(fp,"%d %d\n",HUB,dev.id); //Stampa di tipo ed id
    fprintf(fp,"stato %d\n",dev.state); //Stampa dello stato
    fprintf(fp,"type %d\n",dev.type);   //Stampa del tipo di dispositivo accettato
    fprintf(fp,"%d\n",dev.list->dim);   //Numero di dispositivi figli
    if(dev.list->dim > 0) {
        int i;
        for(i = 0; i < dev.list->dim; i++) //Stampo i nomi dei file dei figli, usando l'array degli id
            fprintf(fp,"%d.txt\n",id[i]);
    }
    fclose(fp);
}

/**Salva il timer nel file specificato
 * file => nome del file di salvataggio
 * dev  => struct con le informazioni sul timer
 * id   => id del figlio, o 0 se non c'è 
 */
void saveTimer(const char file[], const Timer dev, const int id) {
    FILE *fp = fopen(file,"w");
    if(!fp) {   //Controllo sull'apertura del file
        fprintf(stderr,"%d: errore apertura file: %s.",getpid(),strerror(errno));
        return;
    }
    fprintf(fp,"%d %d\n",TIMER,dev.id); //Stampa di tipo ed id
    fprintf(fp,"stato %d\n",dev.state); //Stampa dello stato
    fprintf(fp,"time_begin %ld\n",dev.begin); //Stampa del tempo di accensione
    fprintf(fp,"time_end %ld\n",dev.end);    //Stampa del tempo di spegnimento
    fprintf(fp,"type %d\n",dev.type);
    if(id != 0) {   //Stampo 1 o 0, a seconda della presenza del figlio
        fprintf(fp,"1\n");
        fprintf(fp,"%d.txt\n",id); //Se il figlio c'è, stampo anche il nome del suo file
    }
    else fprintf(fp,"0\n");
    fclose(fp);
}

/**Salva la lampadina nel file specificato
 * file => nome del file di salvataggio
 * dev  => struct con le informazioni sulla lampadina
 */
void saveBulb(const char file[], const Bulb dev) {
    FILE *fp = fopen(file,"w");
    if(!fp) {   //Controllo sull'apertura del file
        fprintf(stderr,"%d: errore apertura file: %s.",getpid(),strerror(errno));
        return;
    }
    fprintf(fp,"%d %d\n",BULB,dev.id); //Stampa di tipo ed id
    fprintf(fp,"stato %d\n",dev.state); //Stampa dello stato on/off
    fprintf(fp,"switch %d\n",dev.mainSwitch); //Stampa dell'interruttore
    fprintf(fp,"time %ld\n",dev.activeTime); //Stampa del tempo di accensione
    fclose(fp);
}

/**Salva la finestra nel file specificato
 * file => nome del file di salvataggio
 * dev  => struct con le informazioni sulla finestra
 */
void saveWindow(const char file[], const Window dev) {
    FILE *fp = fopen(file,"w");
    if(!fp) {   //Controllo sull'apertura del file
        fprintf(stderr,"%d: errore apertura file: %s.",getpid(),strerror(errno));
        return;
    }
    fprintf(fp,"%d %d\n",WINDOW,dev.id); //Stampa di tipo ed id
    fprintf(fp,"stato %d\n",dev.state);  //Stampa dello stato on/off
    fprintf(fp,"open %d\n",dev.openSwitch);  //Stampa dell'interruttore di apertura
    fprintf(fp,"close %d\n",dev.closeSwitch);  //Stampa dell'interruttore di chiusura
    fprintf(fp,"time %ld\n",dev.openTime); //Stampa del tempo di apertura
    fclose(fp);
}

/**Salva del frigorifero nel file specificato
 * file => nome del file di salvataggio
 * dev  => struct con le informazioni sul frigorifero
 */
void saveFridge(const char file[], const Fridge dev) {
    FILE *fp = fopen(file,"w");
    if(!fp) {   //Controllo sull'apertura del file
        fprintf(stderr,"%d: errore apertura file: %s.",getpid(),strerror(errno));
        return;
    }
    fprintf(fp,"%d %d\n",FRIDGE,dev.id); //Stampa di tipo ed id
    fprintf(fp,"stato %d\n",dev.state);  //Stampa dello stato on/off
    fprintf(fp,"open %d\n",dev.mainSwitch);  //Stampa dell'interruttore
    fprintf(fp,"term %d\n",dev.thermostat);      //Stampa del valore del termostato
    fprintf(fp,"time %ld\n",dev.openTime); //Stampa del tempo di apertura
    fprintf(fp,"delay %ld\n",dev.delay); //Stampa del tempo di ritardo di chiusura
    fprintf(fp,"perc %d\n",dev.perc);     //Percentuale di riempimento
    fprintf(fp,"temp %d\n",dev.temp); //Stampa della temperatura interna
    fclose(fp);
}

/**Carica un dispositivo da un file specificato
 * Il parametro è il file da interpretare. La funzione restituisce un numero variabile di stringhe che rappresentano il tipo ed
 * i vari campi del dispositivo, più i nomi dei file dei dispositivi figli, se ve ne sono. La funzione chiamante dovrà ricostruire l'oggetto
 * in base al tipo e DEALLOCARE LE STRINGHE utilizzate
 */
char** loadDevice(const char file[]) {  
    char** fields;
    int id, tipo, stato; //I primi tre campi sono uguali per tutti i dispositivi
    int nFields;
    int i;

    FILE *fp = fopen(file,"r");
    if(!fp) {   //Controllo sull'apertura del file
        fprintf(stderr,"%d: errore apertura file: %s.",getpid(),strerror(errno));
        return NULL;
    }
    fscanf(fp, "%d %d\n", &tipo, &id); //Lettura di tipo ed id;
    fscanf(fp, "stato %d\n", &stato);  //Lettura dello stato
    switch(tipo) {
        case HUB: {
            int type;
            int nDev;
            nFields = 5; //tipo, id, stato, tipo device collegati, numero di figli + evenntuali righe dei figli
            fscanf(fp, "type %d\n", &type); //Tipo di device collegato
            fscanf(fp, "%d\n", &nDev);  //Numero di figli
            nFields += nDev;
            fields = (char**)malloc(nFields * sizeof(char *)); //Alloco nFields campi
            for(i = 0; i < nFields; i++)
                fields[i] = (char*)malloc(NAMESIZE * sizeof(char));
            sprintf(fields[3],"%d",type);   //Scrivo il tipo
            sprintf(fields[4],"%d",nDev);   //Scrivo il numero di figli
            for(i = 5; i < nFields; i++) 
                fscanf(fp,"%s\n", fields[i]); //Se ce ne sono, i nomi dei file dei figli saranno memorizzati nei campi da 5 in poi
        }
        break;
        case TIMER: {
            int nDev, type;
            time_t t_beg, t_end;
            nFields = 7; //tipo, id, stato, tempo di inizio, tempo di fine, tipo, presenza o meno del figlio (0 o 1) + eventuale riga con il nome del figlio
            fscanf(fp, "time_begin %ld\n", &t_beg); //Tempo di inizio
            fscanf(fp, "time_end %ld\n", &t_end);   //tempo di fine
            fscanf(fp, "type %d\n", &type);
            fscanf(fp, "%d\n", &nDev);     //Presenza o meno del figlio
            nFields += nDev;
            fields = (char**)malloc(nFields * sizeof(char *)); //Alloco nFields campi
            for(i = 0; i < nFields; i++)
                fields[i] = (char*)malloc(NAMESIZE * sizeof(char));
            sprintf(fields[3], "%ld", t_beg);   //Scrivo il tempo di inizio
            sprintf(fields[4], "%ld", t_end);   //Scrivo il tempo di fine
            sprintf(fields[5], "%d", type);   //Scrivo il tipo
            sprintf(fields[6], "%d", nDev);   //Scrivo la presenza del figlio
            if(nDev == 1)           //e l'eventuale figlio
                fscanf(fp,"%s\n", fields[7]);
        }
        break;
        case BULB: {    //Il numero di campi è fisso, posso allocare subito le stringhe
            nFields = 5;    
            fields = (char**)malloc(nFields * sizeof(char *)); //Alloco nFields campi
            for(i = 0; i < nFields; i++)
                fields[i] = (char*)malloc(NAMESIZE * sizeof(char));
            fscanf(fp, "switch %s\n", fields[3]); //Leggo il valore dell'interruttore
            fscanf(fp, "time %s\n", fields[4]); //ed il tempo di accensione
        }
        break;
        case WINDOW: { //Il numero di campi è fisso, posso allocare subito le stringhe
            nFields = 6;    
            fields = (char**)malloc(nFields * sizeof(char *)); //Alloco nFields campi
            for(i = 0; i < nFields; i++)
                fields[i] = (char*)malloc(NAMESIZE * sizeof(char));
            fscanf(fp, "open %s\n", fields[3]);     //Interruttore di apertura
            fscanf(fp, "close %s\n", fields[4]);  //Interruttore di chiusura
            fscanf(fp, "time %s\n", fields[5]);   //Tempo di apertura
        }
        break;
        case FRIDGE: { //Il numero di campi è fisso, posso allocare subito le stringhe
            nFields = 9;    
            fields = (char**)malloc(nFields * sizeof(char *)); //Alloco nFields campi
            for(i = 0; i < nFields; i++)
                fields[i] = (char*)malloc(NAMESIZE * sizeof(char));
            
            fscanf(fp, "open %s\n", fields[3]);      //Interruttore di apertura
            fscanf(fp, "term %s\n", fields[4]);    //Interruttore termostato
            fscanf(fp, "time %s\n", fields[5]);    //Tempo di apertura
            fscanf(fp, "delay %s\n", fields[6]);   //Tempo dopo il quale avviene la chiusura automatica
            fscanf(fp, "perc %s\n", fields[7]);    //Percentuale di riempimento
            fscanf(fp, "temp %s\n", fields[8]);    //Temperatura
        }
        break;
        default: return NULL;
    }
    //Le stringhe comuni a tutti i dispositivi vengono scritte alla fine
    sprintf(fields[0],"%d",tipo);
    sprintf(fields[1],"%d",id);
    sprintf(fields[2],"%d",stato);
    
    fclose(fp);
    return fields;
}

void makeHub(char** fields, const int flag) {
    char** argg; //Non posso sapere in anticipo quanti argomenti dovrò passare, quindi alloco dinamicamente
    int nFields = 5; //sei campi di base, più NULL ed eventuali file dei figli
    int nDev; //N di device dell'hub
    int i;

    sscanf(fields[4],"%d",&nDev);
    nFields += nDev;
    argg = (char**)malloc(nFields * sizeof(char*));
    for(i = 0; i <= nFields; i++) { //Aggiungo NULL alla fine 
        argg[i] = (char*)malloc(NAMESIZE * sizeof(char));
    }
    for(i = 1; i < 4; i++) { //I primi tre parametri (id,stato,tipo) sono gli stessi
        strcpy(argg[i], fields[i]);
        free(fields[i]);
    }
    sprintf(argg[4],"%d",flag);
    //In fields, i nomi dei file partono dall'indice 5, mentre in argg dall'indice 5
    for(i = 5; i < 5+nDev; i++) {
        strcpy(argg[i],fields[i]);
        free(fields[i]);
    }
    free(fields[0]);   //Libero il primo elemento e l'array di puntatori
    free(fields);
    argg[nFields] = NULL; //carattere terminatore
    strcpy(argg[0],SRC_HUB);   
    execv(argg[0],argg);    //Eseguo l'hub
}

void makeTimer(char** fields, const int flag) {
    //In queste stringhe copio i parametri di fields, in modo da poter deallocare fields
    char end[NAMESIZE], begin[NAMESIZE], subDev[NAMESIZE], id[NAMESIZE], state[NAMESIZE], strFlag[2], type[NAMESIZE];
    int nDev;
    int i;

    sprintf(strFlag,"%d",flag);
    strcpy(id, fields[1]);
    strcpy(state, fields[2]);
    strcpy(begin, fields[3]);
    strcpy(end, fields[4]);
    strcpy(type, fields[5]);
    sscanf(fields[6],"%d",&nDev);
    if(nDev == 1) { //Il timer ha un figlio
        strcpy(subDev,fields[7]);
        free(fields[7]);
    }
    for(i = 0; i < 7; i++)
        free(fields[i]);
    free(fields);

    if(nDev == 1)
        execlp(SRC_TIMER,SRC_TIMER,id,state,begin,end,type,strFlag,subDev,NULL);
        else execlp(SRC_TIMER,SRC_TIMER,id,state,begin,end,type,NULL);
}

void makeBulb(char** fields) {
    //In queste stringhe copio i parametri di fields, in modo da poterlo deallocare successivamente
    char id[NAMESIZE], state[NAMESIZE], interr[NAMESIZE], activeTime[NAMESIZE];
    int i;

    strcpy(id, fields[1]);
    strcpy(state, fields[2]);
    strcpy(interr, fields[3]);
    strcpy(activeTime, fields[4]);
    for(i = 0; i < 5; i++)
        free(fields[i]);
    free(fields);
    execlp(SRC_BULB,SRC_BULB,id,state,interr,activeTime,NULL);
}

void makeWindow(char** fields) {
    //In queste stringhe copio i parametri di fields, in modo da poterlo deallocare successivamente
    char id[NAMESIZE], state[NAMESIZE], interr1[NAMESIZE], interr2[NAMESIZE], activeTime[NAMESIZE];
    int i;

    strcpy(id, fields[1]);
    strcpy(state, fields[2]);
    strcpy(interr1, fields[3]);
    strcpy(interr2, fields[4]);
    strcpy(activeTime, fields[5]);
    for(i = 0; i < 6; i++)
        free(fields[i]);
    free(fields);
    execlp(SRC_WINDOW,SRC_WINDOW,id,state,interr1,interr2,activeTime,NULL);
}

void makeFridge(char** fields) {
    //In queste stringhe copio i parametri di fields, in modo da poterlo deallocare successivamente
    char id[NAMESIZE], state[NAMESIZE], interr[NAMESIZE], term[NAMESIZE], activeTime[NAMESIZE], delay[NAMESIZE], perc[NAMESIZE], temp[NAMESIZE];
    int i;

    strcpy(id, fields[1]);
    strcpy(state, fields[2]);
    strcpy(interr, fields[3]);
    strcpy(term, fields[4]);
    strcpy(activeTime, fields[5]);
    strcpy(delay, fields[6]);
    strcpy(perc, fields[7]);
    strcpy(temp, fields[8]);
    for(i = 0; i < 9; i++)
        free(fields[i]);
    free(fields);
    execlp(SRC_FRIDGE,SRC_FRIDGE,id,state,interr,term,activeTime,delay,perc,temp,NULL);
}