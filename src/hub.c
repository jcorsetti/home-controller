#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include "messages.h"
#include "utility.h"

extern int errno;
/**
 * Flag globale per segnalare al device che il proprio parent deve comunicare con lui.
 * Viene modificato quando il device riceve un segnale SIGUSR1:
**/
int readParent = 0;

//Informazioni dell' hub
Hub hub;

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
 * Manda un qualsiasi tipo di messaggio a tutti i dispositivi della sua lista
 * ritorna 1 se ha mandato i messaggi
 * 0 se non li ha mandati in caso di lista vuota
**/
int sendAllDev(char *msg);

/**
 * Legge solo messaggi di tipo delete:
 * ritorna il pid del dispositivo con ack positivo se ha updateParent=1
 * ritorna 0 se ha updateParent=0
 * ritorna -1 se tutti gli ack sono negativi
**/
int readAllDevDel();

/**
 * Legge solo messaggi di tipo check:
 * restituisce il messaggio Check con ack positivo
**/
MessageCheck readAllDevCheck();

/**
 * Legge solo messaggi di tipo check.
 * restituisce l'array con l'id dei dispositivi figli
**/ 
int* readAllId();

/**
 * Gestisce l'arrivo di un messaggio di tipo check
**/
void checkDevHub(MessageCheck check_msg, char fifo[]);

/**
 * Gestisce il messaggio 'del' per bulb
**/
void delDevHub(MessageDel del_msg, char fifo[]);

/**
 * Le seguenti funzioni operano su diversi tipi di device.
 * Ogni funzione:
 * Legge tutti messaggio 'info' ricevuto dai figli, estrae il device specifico
 * e controlla se è avvenuto override.
 * Unisce le informazioni dei device in uno unico dove il valore di ogni campo
 * è quello massimo tra tutti i device figli.
 * Restituisce un messaggio 'info' contenente il device appena creato in cui
 * il nome e l'id sono stati sostituiti da quelli dell'hub.
**/
MessageInfo processAllBulbs();
MessageInfo processAllWindows();
MessageInfo processAllFridges();
MessageInfo processAllTimers();

/**
 * Legge solo messaggi di tipo info:
 * ritorna le informazioni relative all'hub in base al tipo di device dei figli.
**/
MessageInfo processAllDevInfo();

/**
 * Legge solo messaggi di tipo info:
 * ritorna il messaggio con ack positivo (ack=1)
 * se non è presente ritorna un messaggio con ack negativo
**/
MessageInfo readAllDevInfo();

//Gestisce il comando 'info'
void infoDevHub(MessageInfo info_msg, char fifo[]);

/**
 * Legge solo messaggi di tipo switch:
 * ritorna un valore di ack:
 * 1 => ack positivo
 * 2 => ack negativo (id non esiste / irraggiungibile)
 * 3 => ack negativo (label non esiste)
 * 4 => ack negativo (pos invalida)
**/
MessageSwitch readAllDevSwitch();

//Gestisce il comando 'switch'
void switchDevHub(MessageSwitch switch_msg, char fifo[]);

//gestisce il comando 'list'
void listDevHub(MessageList list_msg, char fifo[]);

/**
 * Funzione invocata quando si effettua il linking di un dispositivo all'hub o quando si carica un esempio. Ricostruisce l'albero dei dispositivi 
 * (o un singolo dispositivo) dal file relativo al dispositivo con id dato come parametro. Con buildFlag = 0 (utilizzato con l'operazione link) i file sono cercati
 * in PATH_FILE ed eliminati dopo la lettura. Altrimenti (utilizzato quando si lancia il controller con un esempio) i file sono cercati in PATH_EXAMPLE e sono
 * mantenuti dopo la lettura, con il numero di esempio indicato in buildFlag
*/
void buildTree(int id, int buildFlag);

/**
 *Legge solo messaggi di tipo link
 *ritorna 1 in caso di conferma del link
 *ritorna 0 in caso di errore
**/
int readAllDevLink();

//gestisce il comando link
void linkDevHub(MessageLink link_msg, char fifo[]);

/**
 * Legge solo messaggi di tipo update
 * ritorna il tipo di dispositivi controllati:
 * -1(nessun dispositivo di interazione), 3(solo bulb), 4(solo window), 5(solo fridge)
**/
MessageUpdate readAllDevUpdate();

/**
 * Gestisce i messaggi update:
 * se ack = 1, setta il suo tipo con il valore di type
 * se ack = 0, se non ha figli, risponde con ack=0 e type=-1
 *             se ha figli, inoltra il messaggio ai figli e poi inoltra la risposta con ack positivo (se c'è)
 *                  al padre
**/
void updateDevHub(MessageUpdate update_msg, char fifo[]);

int main(int argc, char **argv)
{
    srand(time(0));
    char msg[BUFFSIZE];
    char fifo[20]; 
    int fd;
    hub.list = createDevList();
    
    /**
     * Due possibili configurazioni degli argomenti:
     * 1. Id del dispositivo
     *    In questo caso, agli altri campi è assegnato un valore default.
     * 2. Id, stato, switch, tipo di dispositivo accettato, nomi dei file dei figli
     * In entrambi i casi, il nome è costruito a partire dall'id
    **/
    if(argc == 2) { // argv = [argc,id]
        sscanf(argv[1], "%d", &hub.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(hub.name, "Hub_%03d", hub.id);     //Genero il nome del device
        // I seguenti parametri sono di default (da rivedere)
        hub.state = 0;      
        hub.type=-1;  //-1 NULL, 2 timer, 3 bulb, 4 window, 5 fridge
    }   
    else if(argc >= 5) { // argv = [ argc, id, state, tipo, flag + eventuali figli]
        //printf("%d: seconda conf.\n",getpid());

        sscanf(argv[1], "%d", &hub.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(hub.name, "Hub_%03d", hub.id);     //Genero il nome del device
        sscanf(argv[2], "%d", &hub.state);
        sscanf(argv[3], "%d", &hub.type);
        if(argc > 5) {  //Allora ci sono figli da creare tramite la buildTree
            int i, idSubDev;
            int pid = getpid(); //Viene invocata la buildTree su ogni figlio, pertanto ad ogni ciclo il processo invoca la fork
            int buildFlag; //Flag della buildTree
            sscanf(argv[4],"%d",&buildFlag);
            for(i = 5; i < argc; i++) 
                if(getpid() == pid) { //Guardia per impedire che anche i figli si forkino
                    sscanf(argv[i],"%d.txt",&idSubDev);
                    buildTree(idSubDev,buildFlag);
                }
        }
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
        //si mette in pausa se è spento
        if(!hub.state && !readParent && !readManual){
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

            if(!strncmp(msg,"del", 3)){
                MessageDel del_msg = deSerializeDel(msg);
                delDevHub(del_msg, fifo);
            }else if(!strncmp(msg,"info", 4)){
                MessageInfo info_msg = deSerializeInfo(msg);
                infoDevHub(info_msg, fifo);
            }else if(!strncmp(msg,"list", 4)){
                MessageList list_msg = deSerializeList(msg);
                listDevHub(list_msg, fifo);
            }else if(!strncmp(msg, "switch", 6)){
                MessageSwitch switch_msg = deSerializeSwitch(msg);
                switchDevHub(switch_msg, fifo);
            }else if(!strncmp(msg, "check", 5)){
                MessageCheck check_msg = deSerializeCheck(msg);
                checkDevHub(check_msg, fifo);
            }else if(!strncmp(msg, "link", 4)){
                MessageLink link_msg = deSerializeLink(msg);
                linkDevHub(link_msg, fifo);
            }else if(!strncmp(msg, "update", 6)){
                MessageUpdate update_msg = deSerializeUpdate(msg);
                updateDevHub(update_msg, fifo);
            }
        }//Pipe per il controllo manuale
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
                switchDevHub(switch_msg, MANUAL_FIFO);
            }
        }        
    }
    return 0;
}

/**
 * Manda un qualsiasi tipo di messaggio a tutti i dispositivi della sua lista
 * ritorna 1 se ha mandato i messaggi
 * 0 se non li ha mandati in caso di lista vuota
**/
int sendAllDev(char *msg) {
    if(getDimList(hub.list) == 0)
        return 0;  //Lista vuota
    else {
        SubDev* i = hub.list->head;
        while(i != NULL) {
            kill(i->pid,SIGUSR1);
            int fd = open(i->fifo, O_WRONLY);
            write(fd, msg, BUFFSIZE);
            close(fd);
            i = i->next;
        }
        return 1;
    }
}
// DEL

int readAllDevDel() { 
    int pid=-1; //id del processo da ritornare, se tutti gli ack sono negativi, ritorna 0
    if(getDimList(hub.list) == 0){
        fprintf(stderr,"%d: Errore, il dispositivo %d non ha dispositivi controllati\n.", getpid(), hub.id);
        return pid;
    }else{
        char msg[BUFFSIZE];
        SubDev* i = hub.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageDel struct_msg = deSerializeDel(msg);
            close(fd);
            
            if(struct_msg.ack==1){
                pid=struct_msg.id;
                
                if(struct_msg.updateParent==0){
                    pid=0;
                }
            }   
            //anche se ha gia trovato un dispositivo con ack positivo, deve cmq leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
        }
        return pid;
    }
}

//Funzione delete per gli hub
void delDevHub(MessageDel del_msg, char fifo[]) { 
	
    int ack,id;
    id = del_msg.id;
    int updateParent_c; //variabile per il messaggio da inviare al figlio
    int updateParent_p; //variabile per il messaggio da inviare al padre
    int se_eliminare; //0=non terminerà, 1=terminerà alla fine
    
    
    if(hub.id == del_msg.id || del_msg.id == 0) { //trovato l'id del device
        if(del_msg.save==1) {
            char file[20];
            sprintf(file,"%s%d.txt", PATH_FILE, hub.id);
            if(getDimList(hub.list) == 0) {
                //Se l'hub non ha figli, devo solo salvarlo
                saveHub(file,hub,NULL);
            } else {
                //Devo chiedere gli id dei figli
                char check_msg[BUFFSIZE];
                MessageCheck check_struct = createCheck(0,0);
                serializeCheck(check_msg,check_struct);
                //Gli id si ottengono tramite uno specifico messaggio check
                sendAllDev(check_msg);
                //Lettura degli id
                int* ids = readAllId();

                //Salvo l'hub con i file dei figli
                saveHub(file,hub,ids);
            }
        }
        //il processo terminerà e se serve invierà ack positivo
        ack=1; //ack positivo per rispondere al padre;
        se_eliminare=1;
        updateParent_c=0; //il figlio non dovra rispondere
        updateParent_p=1; //il parent dovrà aggiornare la lista dei figli se attende risposta
        id=0; //se ha figli, si dovranno eliminare
    }else{
        //il processo non terminerà e manderà ack negativo
        ack=2; //finché non trova l'id , l'ack è negativo
        se_eliminare=0;
        updateParent_c=1; //il figlio dovrà rispondere
        updateParent_p=0; //il padre non dovrà aggiornarsi la lista
    }

    MessageDel msg_c= createDel(id, del_msg.save); //creo il messaggio di delete per i figli
    if(del_msg.save){ //se i figli si devono salvare, mandano risposta al padre
        updateParent_c = 1;
    }
    msg_c.updateParent=updateParent_c;
    
    char serialised_msg_c[BUFFSIZE]; //dim migliore?
    serializeDel(serialised_msg_c, msg_c);//serializzo il messaggio

    
    if(sendAllDev(serialised_msg_c)){ //Se ha dispositivi nella lista manda i messaggi
        if(updateParent_c){ //se i figli dovevano rispondere, si mette in ascolto
            int pid = readAllDevDel(hub.list);
            //pid=-1  ->tutti gli ack sono negativi
            //pid=0   ->c'è un ack positivo ma con updateParent=0
            //pid=id del processo da togliere della lista ->ack positivo con updateParent=1
            if(pid!=-1){//ack positivo, un dispositivo è stato eliminato
                if(pid!=0){//updateParent=1, il dispositivo eliminato era un figlio, devo toglierlo dalla lista
                    removeDevByPid(hub.list,pid);
                    if(getDimList(hub.list) == 0)
                        hub.type = EMPTY;   //Il tipo torna a -1
                    wait(NULL);//elimina il processo defunct (rimane zoombie se non chiamo la wait)
                }
                ack=1;
            }else{
                ack=2;
            }
            
        }
    }
    
    if(del_msg.updateParent){ //deve rispondere al padre
        MessageDel msg_p= createDel(id, 0); //creo il messaggio di risposta per il padre
        msg_p.ack=ack;
        msg_p.updateParent=updateParent_p;
        msg_p.id=getpid();
        char serialised_msg_p[BUFFSIZE]; //dim migliore?
        serializeDel(serialised_msg_p,msg_p);//trasformo il messaggio in stringa
        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg_p, BUFFSIZE);//mando il messaggio al padre
        close(fd);
    }
    if(se_eliminare){
        //printf("Dispositivo con ID %d eliminato\n",hub.id);
        exit(0);
    }
    //se non ha piu figli, il tipo diventa -1 (vuoto)
    if(getDimList(hub.list)==0){
        hub.type = -1;
    }
}
//CHECK

MessageCheck readAllDevCheck() {
    MessageCheck retInfo;
    retInfo.ack = 2; //Se non si trovano ack positivi viene restituito ack negativo
    if(getDimList(hub.list) == 0) {
        //printf("Errore, nessuno da leggere.\n");
        return retInfo;
    } else {
        char msg[BUFFSIZE];
        SubDev* i = hub.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageCheck struct_msg = deSerializeCheck(msg);
            close(fd);
            //Se l'ack è positivo devo restituirlo perchè contiene le info del device
            if(struct_msg.ack == 1 || struct_msg.ack == 3)
                retInfo = struct_msg;
            //Se il proprio id corrisponde ad un ancestor in un check confermato, si è in errore
            if(struct_msg.ack == 1 && struct_msg.idAncestor == hub.id)
                retInfo.ack = 3; 
            //Anche se ha già trovato un dispositivo con ack positivo, deve comunque leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
        }
        return retInfo;
    }
}

void checkDevHub(MessageCheck check_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    if(hub.id == check_msg.id || check_msg.id == 0) {
        //Trovato l'id cercato
        check_msg.ack = 1;
        check_msg.id = hub.id;
        check_msg.pid = getpid();
        check_msg.type = hub.type;
        check_msg.isControl = 1;
        check_msg.oldestParent = getpid();
        //genero il messaggio stringa
        serializeCheck(serialised_msg, check_msg);
        //invio il messaggio
        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);
        close(fd);
    } else {
        serializeCheck(serialised_msg, check_msg);
        //invio il messaggio ai figli
        sendAllDev(serialised_msg);
        //ricevo risposta dai figli ed inoltro al parent
        check_msg = readAllDevCheck(hub.list);
        check_msg.oldestParent = getpid();
        serializeCheck(serialised_msg, check_msg);
        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);
        close(fd);
    }
}

int* readAllId() {
    int* ids = NULL;
    int j = 0;

    if(getDimList(hub.list) == 0) {
        fprintf(stderr,"%d: Errore, il dispositivo %d non ha dispositivi controllati\n.", getpid(), hub.id);
    } else {
        ids = (int*)malloc(getDimList(hub.list) * sizeof(int));
        char msg[BUFFSIZE];
        SubDev* i = hub.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageCheck struct_msg = deSerializeCheck(msg);
            close(fd);
            //In teoria tutti gli ack sono positivi
            ids[j] = struct_msg.id;
            j++;
            i = i->next;
        }
    }
    return ids;
}

// SWITCH

void switchDevHub(MessageSwitch switch_msg, char fifo[]) {
    MessageSwitch retSwitch;
    char serialised_msg[BUFFSIZE];
    retSwitch.ack = 3;      //Di default l'ack di risposta è negativo

    //Controllo se il messaggio è per l'hub
    if(hub.id == switch_msg.id || switch_msg.id == 0){        
        //Setto l'id a 0 in modo da ordinare ad ogni figlio di eseguire il comando switch
        switch_msg.id = 0;

        serializeSwitch(serialised_msg, switch_msg);

        //Manda il messaggio a tutti i suoi figli
        if(!sendAllDev(serialised_msg)) {
            //Se non ha figli l'ack viene settato a negativo (label invalido)
            //perchè non sono presenti interruttori
            retSwitch.ack = 4;
        }else {
            //Se invece ho figli leggo i messaggi di risposta dai figli
            retSwitch = readAllDevSwitch(hub.list);
            //Se c'è stato un cambio di stato devo aggiornare lo stato dell'hub
            //con il valore all'interno dello switch
            if((retSwitch.ack == 1)  && retSwitch.newState != -1) {
                hub.state = retSwitch.newState;
            }
        }        
    }

    //Rimando il messaggio al padre con ack specificato
    serializeSwitch(serialised_msg, retSwitch);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

MessageSwitch readAllDevSwitch() { 
    MessageSwitch retSwitch;    //Messaggio da ritornare
    retSwitch.ack = 3;          //Di default l'ack ritornato al padre è negativo (id invalido / irraggiungibile)
    
    if(getDimList(hub.list) == 0){
        fprintf(stderr,"%d: Errore, il dispositivo %d non ha dispositivi controllati\n.", getpid(), hub.id);
        return retSwitch;
    }else{
        char msg[BUFFSIZE];
        SubDev* i = hub.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageSwitch struct_msg = deSerializeSwitch(msg);
            close(fd);
            /**
             * Controllo se l'ack del messaggio ricevuto è diverso da quello di default (id non esiste)
             * Se però l'ack gia salvato indica un cambio di stato ha la priorità su tutti
             * gli altri ack.
            **/
            if(struct_msg.ack != 3 && retSwitch.ack != 1){
                retSwitch = struct_msg;
            }
            //Anche se ha gia trovato un dispositivo con ack positivo, deve comunque leggere 
            //dalla pipe degli altri figli per sbloccarli
            i = i->next;
        }
        return retSwitch;
    }
}

// INFO

void infoDevHub(MessageInfo info_msg, char fifo[]) {
    
    char serialised_msg[BUFFSIZE];

    if(hub.id == info_msg.id || info_msg.id == 0) {
        //Setto l'id a 0 in modo da ordinare ad ogni figlio di inviare le proprie informazioni
        info_msg.id = 0;
        serializeInfo(serialised_msg, info_msg);

        MessageInfo retInfo;
        if(!sendAllDev(serialised_msg)) {
            //Se non ho figli invio info dell'hub
            retInfo.ack = 1;
            retInfo.type = HUB;
            serializeHub(retInfo.device, hub);
        }else {
            retInfo = processAllDevInfo(hub.list);
        }
        //Incapsulo all'interno del messaggio le info del device
        serializeInfo(serialised_msg, retInfo);
        //Invio al padre il messaggio
        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);
        close(fd);

    }else {
        //Inoltro il messaggio ai figli se sono presenti
        serializeInfo(serialised_msg, info_msg);
        if(!sendAllDev(serialised_msg)) {
            
            info_msg.ack = 2;
            serializeInfo(serialised_msg, info_msg);
            //Invio al padre il messaggio negativo
            int fd = open(fifo, O_WRONLY);
            write(fd, serialised_msg, BUFFSIZE);
            close(fd);
            return;
        }
        
        //Cerco il messaggio info con ack positivo (se esiste)
        MessageInfo retInfo = readAllDevInfo();
        
        //Non devo controlare se  positivo o negativo e posso semplicemente inoltrarlo
        serializeInfo(serialised_msg, retInfo);

        int fd = open(fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);
        close(fd);
    }    
}

MessageInfo processAllDevInfo(const DevList* list) { 
    MessageInfo retInfo;
    retInfo.ack = 2; //Se non si trovano ack positivi viene restituito ack negativo
    if(getDimList(list) == 0){
        fprintf(stderr,"%d: Errore, il dispositivo %d non ha dispositivi controllati\n.", getpid(), hub.id);
    }else{
        switch(hub.type) {
            case BULB: {
                retInfo = processAllBulbs();
            }
            break;
            case WINDOW: {
                retInfo = processAllWindows();
            }
            break;
            case FRIDGE: {
                retInfo = processAllFridges();
            }
            break;
            //Se l'hub è vuoto (cioè non ha figli che sono bulb, window o fridge) allora prova a cercare 
            //dei timer tra i figli. Se non ne trova ritorna solo le proprie informazioni al padre
            case EMPTY: {
                retInfo = processAllTimers();
            }
            break;

            default:{
                fprintf(stderr,"%d: Errore in processAllDevInfo, device non riconosciuto\n.", getpid());
                retInfo.type = -1;
                retInfo.ack = 1;
            }
        }
    }

    return retInfo;    
}

MessageInfo processAllBulbs() {
    int override = 0;       //Indica se c'è override
    int stateValue;         //Stato del device: 0 => off, 1 => on, 2 => off (override), 3 => on (override)
    int sumActiveTime = 0;  //Salva la somma dei valori di activeTime per il calcolo della media
    int count = 0;          //Salva il numero di devices

    char serialized_msg[BUFFSIZE];
    MessageInfo retInfo;
    SubDev* i = hub.list->head;

    Bulb retBulb;

    while(i != NULL) {
        int fd = open(i->fifo, O_RDONLY);
        read(fd, serialized_msg, BUFFSIZE);
        close(fd);
        MessageInfo nextInfo = deSerializeInfo(serialized_msg);
        //Se le info ricevute sono relative ad un hub allora l'hub ricevuto è vuoto e viene ignorato
        if(strncmp(nextInfo.device, "hub", 3) && strncmp(nextInfo.device, "timer", 5)) {
            //Estrae il prossimo bulb
            retBulb = deSerializeBulb(nextInfo.device);
            //Se lo stato di un device è diverso da quello dell'hub c'è override
            if(retBulb.state != hub.state) {
                override = 1;
            }
            //Aggiorna la somma
            sumActiveTime += retBulb.activeTime;
            //Conto il numero di device letti
            count++;
        }
        i = i->next;
    }

    //Controllo se ho trovato almeno un fridge
    if(count > 0) {
        //Se ho trovato almeno un bulb lo ritorno
        //Controlla se c'è override
        if(override)
            stateValue = hub.state ? 3 : 2;
        else
            stateValue = hub.state;
        
        //Assegna i valori al bulb da ritornare
        retBulb.state = stateValue;
        retBulb.activeTime = sumActiveTime / count;

        //Sostituisce nome e id con quelli dell'hub
        retBulb.id = hub.id;
        strcpy(retBulb.name, hub.name);
        //Inserisce il bulb nel messaggio indicando il tipo di device all'interno
        retInfo.type = BULB;
        serializeBulb(retInfo.device, retBulb);
    }else {
        //Se non ho trovato bulbs e quindi ho solo figli che sono hub o timer vuoti invio info dell'hub
        retInfo.type = HUB;
        serializeHub(retInfo.device, hub);
    }
    retInfo.ack = 1;
    return retInfo;
}

MessageInfo processAllWindows() {
    int override = 0;       //Indica se c'è override
    int stateValue;         //Stato del device: 0 => off, 1 => on, 2 => off (override), 3 => on (override)
    int sumOpenTime = 0;    //Salva la somma dei valori di openTime per il calcolo della media
    int count = 0;          //Salva il numero di devices
    
    char serialized_msg[BUFFSIZE];
    MessageInfo retInfo;
    SubDev* i = hub.list->head;

    Window retWindow;

    while(i != NULL) {
        int fd = open(i->fifo, O_RDONLY);
        read(fd, serialized_msg, BUFFSIZE);
        close(fd);
        MessageInfo nextInfo = deSerializeInfo(serialized_msg);
        //Se le info ricevute sono relative ad un hub allora l'hub ricevuto è vuoto e viene ignorato
        if(strncmp(nextInfo.device, "hub", 3) && strncmp(nextInfo.device, "timer", 5)) {
            //Estrae la prossima window
            retWindow = deSerializeWindow(nextInfo.device);
            //Se lo stato di un device è diverso da quello dell'hub c'è override
            if(retWindow.state != hub.state)
                override = 1;
            //Aggiorna la somma
            sumOpenTime += retWindow.openTime;
            //Conto il numero di device letti
            count++;
        }
        i = i->next;
    }

    //Controllo se ho trovato almeno un fridge
    if(count > 0) {
        //Se ho trovato almeno un fridge lo ritorno

        //Controlla se c'è override
        if(override)
            stateValue = hub.state ? 3 : 2;
        else
            stateValue = hub.state;

        //Assegna i valori alla window da ritornare
        retWindow.state = stateValue;
        retWindow.openTime = sumOpenTime / count;

        //Sostituisce nome e id con quelli dell'hub
        retWindow.id = hub.id;
        strcpy(retWindow.name, hub.name);
        //Inserisce la window nel messaggio indicando il tipo di device all'interno
        retInfo.type = WINDOW;
        serializeWindow(retInfo.device, retWindow);
    }else {
        //Se non ho trovato windows e quindi ho solo figli che sono hub o timer vuoti invio info dell'hub
        retInfo.type = HUB;
        serializeHub(retInfo.device, hub);
    }
    retInfo.ack = 1;
    return retInfo;
}

MessageInfo processAllFridges() {
    int override = 0;      //Indica se c'è override
    int stateValue;        //Stato del device: 0 => off, 1 => on, 2 => off (override), 3 => on (override)
    int sumOpenTime = 0;   //Salva la somma dei valori di openTime per il calcolo della media
    int sumPerc = 0;       //Salva la somma dei valori di perc per il calcolo della media
    int sumTemp = 0;       //Salva la somma dei valori di temp per il calcolo della media
    int sumDelay = 0;      //Salva la somma dei valori di delay per il calcolo della media
    int count = 0;         //Salva il numero di devices

    char serialized_msg[BUFFSIZE];
    MessageInfo retInfo;
    SubDev* i = hub.list->head;

    Fridge retFridge;
    
    while(i != NULL) {
        int fd = open(i->fifo, O_RDONLY);
        read(fd, serialized_msg, BUFFSIZE);
        close(fd);
        MessageInfo nextInfo = deSerializeInfo(serialized_msg);
        //Se le info ricevute sono relative ad un hub allora l'hub ricevuto è vuoto e viene ignorato
        if(strncmp(nextInfo.device, "hub", 3) && strncmp(nextInfo.device, "timer", 5)) {
            //Estrae il prossimo fridge
            retFridge = deSerializeFridge(nextInfo.device);
            //Se lo stato di un device è diverso da quello dell'hub c'è override
            if(retFridge.state != hub.state)
                override = 1;
            //Aggiorna la somma
            sumOpenTime += retFridge.openTime;
            sumPerc += retFridge.perc;
            sumTemp += retFridge.temp;
            sumDelay += retFridge.delay;
            //Conto il numero di device letti
            count++;
        }
        i = i->next;
    }
    
    //Controllo se ho trovato almeno un fridge
    if(count > 0) {
        //Se ho trovato almeno un fridge lo ritorno
        
        //Controlla se c'è override
        if(override)
            stateValue = hub.state ? 3 : 2;
        else
            stateValue = hub.state;

        //Assegna i valori al fridge da ritornare
        retFridge.state = stateValue;
        retFridge.openTime = sumOpenTime / count;
        retFridge.perc = sumPerc / count;
        retFridge.temp = sumTemp / count;
        retFridge.delay = sumDelay / count;
        //Sostituisce nome e id con quelli dell'hub
        retFridge.id = hub.id;
        strcpy(retFridge.name, hub.name);
        //Inserisce il fridge nel messaggio indicando il tipo di device all'interno
        retInfo.type = FRIDGE;
        serializeFridge(retInfo.device, retFridge);
    }else {
        //Se non ho trovato fridges e quindi ho solo figli che sono hub o timer vuoti invio info dell'hub
        retInfo.type = HUB;
        serializeHub(retInfo.device, hub);
    }

    //Inserisce la window nel messaggio indicando il tipo di device all'interno
    retInfo.ack = 1;
    return retInfo;
}

MessageInfo processAllTimers() {
    int maxBegin = -1;      //Salva il massimo valore di begin
    int maxEnd = -1;        //Salva il massimo valore di end
    int count = 0;          //Salva il numero di devices
    
    char serialized_msg[BUFFSIZE];
    MessageInfo retInfo;
    SubDev* i = hub.list->head;

    Timer retTimer;

    while(i != NULL) {
        int fd = open(i->fifo, O_RDONLY);
        read(fd, serialized_msg, BUFFSIZE);
        close(fd);
        MessageInfo nextInfo = deSerializeInfo(serialized_msg);
        //Se le info ricevute sono relative ad un hub allora l'hub ricevuto è vuoto e viene ignorato
        if(strncmp(nextInfo.device, "hub", 3)) {
            //Estrae il prossimo timer
            retTimer = deSerializeTimer(nextInfo.device);
            //Salva il massimo
            maxBegin = max(maxBegin, retTimer.begin);
            maxEnd = max(maxEnd, retTimer.end);
            //Conto il numero di device letti
            count++;
        }
        i = i->next;
    }
    //Controllo se ho trovato almeno un timer
    if(count > 0) {
        //Se ho trovato almeno un timer lo ritorno
        //Assegna i valori al timer da ritornare
        retTimer.begin = maxBegin;
        retTimer.end = maxEnd;
        //Sostituisce nome e id con quelli dell'hub
        retTimer.id = hub.id;
        strcpy(retTimer.name, hub.name);
        //Inserisce il timer nel messaggio indicando il tipo di device all'interno
        retInfo.type = TIMER;
        serializeTimer(retInfo.device, retTimer);
    }else {
        //Se non ho trovato timers e quindi ho solo figli che sono hub vuoti invio info dell'hub
        retInfo.type = HUB;
        serializeHub(retInfo.device, hub);
    }
    
    retInfo.ack = 1;
    return retInfo;
}


MessageInfo readAllDevInfo() { 
    MessageInfo retInfo;
    retInfo.ack = 2; //Se non si trovano ack positivi viene restituito ack negativo
    if(getDimList(hub.list) == 0){
        fprintf(stderr,"%d: Errore, il dispositivo %d non ha dispositivi controllati\n.", getpid(), hub.id);
        return retInfo;
    }else{
        char msg[BUFFSIZE];
        SubDev* i = hub.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageInfo struct_msg = deSerializeInfo(msg);
            close(fd);
            //Se l'ack è positivo devo restituirlo perchè contiene le info del device
            if(struct_msg.ack == 1)
                retInfo = struct_msg;
            //Anche se ha gia trovato un dispositivo con ack positivo, deve comunque leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
        }
        return retInfo;
    }
}

void listDevHub(MessageList list_msg, char fifo[]) {
    //Dato il livello n, lascia n spazi prima di stamparsi
    int i;
    for(i=0;i<list_msg.level;i++){
        printf("\t");
    }
    char state[5];
    if(hub.state){
        strcpy(state,"ON");
    }else{
        strcpy(state,"OFF");
    }
    if(hub.type == -1) {
        strcpy(state,"");
    }
    printf("| -%s  %s\n",hub.name,state);
    list_msg.level++; //aumento il livello per gli eventuali figli
    char serialised_msg[BUFFSIZE];

    if(getDimList(hub.list)!=0){//se ha figli, inoltra il messaggio
        char response[BUFFSIZE];
        serializeList(serialised_msg,list_msg);
        //per ogni figlio manda il messaggio e aspetta subito la risposta
        SubDev* i = hub.list->head;
        while(i != NULL) {
            kill(i->pid,SIGUSR1);
            int fd = open(i->fifo, O_WRONLY);
            write(fd, serialised_msg, BUFFSIZE);
            close(fd);
            fd=open(i->fifo, O_RDONLY);
            read(fd,response,BUFFSIZE);
            close(fd);
            i = i->next;
        }
    }
    //risponde al padre
    list_msg.ack=1;
    serializeList(serialised_msg, list_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE );
    close(fd);
}

void buildTree(int id, int buildFlag) {
    char path[BUFFSIZE];  //nome del file
    char** fields;  //campi letti da file
    int tipo;

    if(buildFlag == 0)
        sprintf(path,"%s%d.txt", PATH_FILE, id);    //Cerco nei file temporanei
        else sprintf(path,"%sex%d/%d.txt", PATH_EXAMPLE, buildFlag, id);    //ALtrimenti fra gli esempi, nella cartella indicata da buildFlag
    fields = loadDevice(path);  //Caricamento del device
    sscanf(fields[0],"%d",&tipo);
    if(buildFlag == 0)
        deleteFile(path);

    int pid = fork();
    if(pid == 0) {  //Figlio, copia le stringhe e esegue l'exec del relativo dispositivo
        //MIRRORING:
        //Al primo collegamento, il dispositivo assume lo stesso stato dell'hub
        sprintf(fields[2], "%d", hub.state); 
        switch(tipo) {
            case HUB: makeHub(fields,buildFlag);
            break;
            case TIMER: makeTimer(fields,buildFlag);
            break;
            case BULB: makeBulb(fields);
            break;
            case WINDOW: makeWindow(fields);
            break;
            case FRIDGE: makeFridge(fields);
            break;
            default: {}
        }
        if(hub.type == EMPTY)   // Se l'hub non ha tipo (ovvero non ha figli) assume il tipo del figlio collegato
            hub.type = tipo;    // si noti che si presuppone che i tipi siano stati precedentemente controllati tramite check
    }
    else if(pid > 0) {  //Padre, aggiorna la lista e crea la fifo
        char tmpfifo[20];
        sprintf(tmpfifo, "%s%d_%d", PATH_FIFO, getpid(), pid);
        mkfifo(tmpfifo, 0666);
        addToList(hub.list, pid, tmpfifo);
    }
    else {
        fprintf(stderr,"%d: impossibile aggiungere dispositivo: %s\n",getpid(),strerror(errno));
        return;
    }
}

int readAllDevLink() { 
    int da_ritornare=0;
    if(getDimList(hub.list) == 0){
        return da_ritornare;
    }else{
        char msg[BUFFSIZE];
        SubDev* i = hub.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageLink struct_msg = deSerializeLink(msg);
            close(fd);
            //Se l'ack è positivo devo restituirlo perchè contiene le info del device
            if(struct_msg.ack == 1){
                da_ritornare = 1;
            }
            //Anche se ha gia trovato un dispositivo con ack positivo, deve comunque leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
        }    
    }
    return da_ritornare;
}

void linkDevHub(MessageLink link_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    //controllo è l'id dell'hub
    if(link_msg.id2==hub.id){
        buildTree(link_msg.id1,0);
        link_msg.ack=1;
        //hub.type=link_msg.type_id1;
    }else{
        serializeLink(serialised_msg, link_msg);
        if(sendAllDev(serialised_msg)){//Se ha figli
            link_msg.ack=readAllDevLink();
        }
    }

    serializeLink(serialised_msg, link_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

void updateDevHub(MessageUpdate update_msg, char fifo[]){
   
    //se ack=1, setta il nuovo type
    if(update_msg.ack){
        hub.type=update_msg.type;
    }else{//se ack=0, setta il type al messaggio a -1
        update_msg.type=-1;
    }
    char serialised_msg[BUFFSIZE];
    
    serializeUpdate(serialised_msg,update_msg);
    //se ha figli, inoltra il messaggio
    if(sendAllDev(serialised_msg)){
        update_msg = readAllDevUpdate();
    }
    
    serializeUpdate(serialised_msg, update_msg);
    //risponde al padre
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}


MessageUpdate readAllDevUpdate() {
    MessageUpdate da_ritornare;
    da_ritornare = createUpdate(0,0);
    da_ritornare.type=-1;
    if(getDimList(hub.list) == 0) {
        return da_ritornare;
    } else {
        char msg[BUFFSIZE];
        SubDev* i = hub.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageUpdate struct_msg = deSerializeUpdate(msg);
            close(fd);
            //Se c'è almeno un dispositivo di interazione, resituisco il msg settando il tipo trovato
            if(struct_msg.type != -1){
                da_ritornare = struct_msg;
            }
            //Anche se ha già trovato un dispositivo con ack positivo, deve comunque leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
            
        }
        return da_ritornare;
    }
}