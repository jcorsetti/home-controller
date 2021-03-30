//Simula una lampadina
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "messages.h"
#include "utility.h"

extern int errno;
/**
 * Flag globale per segnalare al device che il proprio parent deve comunicare con lui.
 * Viene modificato quando il device riceve un segnale SIGUSR1:
**/
int readParent = 0;

//Informazioni del timer
Timer timer;

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

//Invia un messaggio al device figlio
int sendDev(char *msg);

/**
 * Gestisce il messaggio 'del' per timer
**/
void delDevTimer(MessageDel del_msg, char fifo[]);

/**
 *Legge il messaggio 'del' per timer
 * Ritorna -1 se l'ack del figlio e nagativo oppure 1 altrimenti 
**/
int readDevDel();

/**
 * Gestisce il messaggio 'info' per timer
 * Risponde al padre con le informazioni del device o un ack negativo
**/
void infoDevTimer(MessageInfo info_msg, char fifo[]);

/**
 * Gestisce il messaggio 'check' per tiemr
 * Risponde al padre con le proprie informazioni
**/
void checkDevTimer(MessageCheck check_msg, char fifo[]);

/**
 * Legge il messaggio 'info' in arrivo dal device figlio
 * Ritorna un messaggio 'info' con le informazioni del device o un ack negativo
**/
MessageInfo readDevInfo();

/**
 * Gestisce il messaggio 'switch' per timer
**/
void switchDevTimer(MessageSwitch switch_msg, char fifo[]);

//Gestisce il messaggio 'list'
void listDevTimer(MessageList list_msg, char fifo[]);

/**
 * Funzione invocata quando si effettua il linking di un dispositivo al timer o quando si carica un esempio. Ricostruisce l'albero dei dispositivi 
 * (o un singolo dispositivo) dal file relativo al dispositivo con id dato come parametro. Con buildFlag = 0 (utilizzato con l'operazione link) i file sono cercati
 * in PATH_FILE ed eliminati dopo la lettura. Altrimenti (utilizzato quando si lancia il controller con un esempio) i file sono cercati in PATH_EXAMPLE e sono
 * mantenuti dopo la lettura, con il numero di esempio indicato in buildFlag
*/
void buildTree(int id, int buildFlag);

//gestisce il comando link
void linkDevTimer(MessageLink link_msg, char fifo[]);

/**
 * legge solo un messaggio di tipo link
 * ritorna 1 per conferma
 * ritorna 0 in caso di errore
**/
int readDevLink();

/**
 * Gestisce i messaggi update:
 * se ack = 1, setta il suo tipo con il valore di type
 * se ack = 0, se non ha un figlio, risponde con ack=0 e type=-1
 *             se ha un figlio, gli inoltra il messaggio e poi inoltra la risposta con ack positivo (se c'è)
 *                  al padre
**/
void updateDevTimer(MessageUpdate update_msg, char fifo[]);

/**
 * Legge solo messaggi di tipo update
 * ritorna il tipo di dispositivo controllato:
 * -1(nessun dispositivo di interazione), 3(solo bulb), 4(solo window), 5(solo fridge)
**/
MessageUpdate readDevUpdate();


int main(int argc, char* argv[]) {
    char msg[BUFFSIZE];
    char fifo[20]; 
    int fd;

    /**
     * Due possibili configurazioni degli argomenti:
     * 1. Id del dispositivo
     *    In questo caso, agli altri campi è assegnato un valore default.
     * 2. Id, stato, interruttore, begin, end, più nome del file dell'eventuale figlio
     * In entrambi i casi, il nome è costruito a partire dall'id
    **/
    if(argc == 2) { // argv = [argc, id]
        sscanf(argv[1], "%d", &timer.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(timer.name, "Timer_%03d", timer.id);     //Genero il nome del device
        timer.state = 0;           
        timer.begin = time(NULL) + 120;
        timer.end = time(NULL) + 180;
        timer.type = EMPTY;
    }   
    else if(argc == 6 || argc == 8) { // argv = [argc, id, switch, begin, end, type]
        sscanf(argv[1], "%d", &timer.id);            //Copio l'id
        sprintf(fifo, "%s%d_%d", PATH_FIFO, getppid(), getpid());  //Genero il nome della fifo di comunicazione
        sprintf(timer.name, "Timer_%03d", timer.id);     //Genero il nome del device
        sscanf(argv[2], "%d", &timer.state);
        sscanf(argv[3], "%ld", &timer.begin);
        sscanf(argv[4], "%ld", &timer.end);
        sscanf(argv[5], "%d", &timer.type);
        //C'è anche un figlio, va gestita la sua creazione tramite buildTree
        if(argc == 8) { // argv = [argc, id, state, begin, end, type, buildFlag, eventualeFiglio]
            int idSubDev, buildFlag;
            sscanf(argv[6],"%d",&buildFlag);
            sscanf(argv[7],"%d.txt",&idSubDev);
            buildTree(idSubDev,buildFlag);
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

        if(readParent ) {
            readParent = 0;

            fd = open(fifo, O_RDONLY);
            if(fd == -1){
                fprintf(stderr,"%d: Errore di comunicazione con il dispositivo controllore %s: %s\n",getpid(), fifo ,strerror(errno)); //Errore con il percorso della pipe
                exit(-1);
            }
            read(fd, msg, BUFFSIZE);

            close(fd);

            if(!strncmp(msg, "del", 3)){
                MessageDel del_msg = deSerializeDel(msg);
                delDevTimer(del_msg, fifo);
            }else if(!strncmp(msg,"info", 4)){
                MessageInfo info_msg = deSerializeInfo(msg);
                infoDevTimer(info_msg, fifo);
            }else if(!strncmp(msg,"list", 4)){
                MessageList list_msg=deSerializeList(msg);
                listDevTimer(list_msg, fifo);
            }else if(!strncmp(msg,"switch", 6)){
                MessageSwitch switch_msg = deSerializeSwitch(msg);
                switchDevTimer(switch_msg, fifo);
            }else if(!strncmp(msg,"check", 5)){
                MessageCheck check_msg = deSerializeCheck(msg);
                checkDevTimer(check_msg, fifo);
            }else if(!strncmp(msg, "link", 4)){
                MessageLink link_msg = deSerializeLink(msg);
                linkDevTimer(link_msg, fifo);
            }else if(!strncmp(msg, "update", 6)){
                MessageUpdate update_msg = deSerializeUpdate(msg);
                updateDevTimer(update_msg, fifo);
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
                switchDevTimer(switch_msg, MANUAL_FIFO);
            }
        }
        //Se il tempo corrente corrisponde a quello di begin
        if(time(NULL) == timer.begin) {
            //Crea il messaggio di tipo switch
            MessageSwitch switch_msg = createSwitch(0, "allSwitch", "on");
            char serialised_msg[BUFFSIZE];
            //Trasforma il messaggio in stringa
            serializeSwitch(serialised_msg, switch_msg);
            //Manda il messaggio a tutti i suoi figli
            if(!sendDev(serialised_msg)) {
                //Se non ha figli modifica solo il suo stato
                timer.state = 1;
            }else {
                //Legge il messaggio ricevuto dal figlio
                int fd = open(timer.dev->fifo, O_RDONLY);
                read(fd, serialised_msg, BUFFSIZE);
                close(fd);
                switch_msg = deSerializeSwitch(serialised_msg);
                //Se è avvenuto un cambio di stato deve aggiornare lo stato del timer
                //con il valore all'interno dello switch
                if(switch_msg.ack == 1 && switch_msg.newState != -1) {
                    timer.state = switch_msg.newState;
                }
            }
        }
        //Se il tempo corrente corrisponde a quello di end
        if(time(NULL) == timer.end) {
            //Creo il messaggio di tipo switch
            MessageSwitch switch_msg = createSwitch(0, "allSwitch", "off");
            char serialised_msg[BUFFSIZE];
            //Trasformo il messaggio in stringa
            serializeSwitch(serialised_msg, switch_msg);
            //Manda il messaggio a tutti i suoi figli
            if(!sendDev(serialised_msg)) {
                //Se non ha figli modifica solo il suo stato
                timer.state = 0;
            }else {
                //Legge il messaggio ricevuto dal figlio
                int fd = open(timer.dev->fifo, O_RDONLY);
                read(fd, serialised_msg, BUFFSIZE);
                close(fd);
                switch_msg = deSerializeSwitch(serialised_msg);
                //Se è avvenuto un cambio di stato deve aggiornare lo stato del timer
                //con il valore all'interno dello switch
                if(switch_msg.ack == 1 && switch_msg.newState != -1) {
                    timer.state = switch_msg.newState;
                }
            }
        }
    }
}

int sendDev(char *msg) {
    if(!timer.dev) return 0;  //Non ha un figlio
        kill(timer.dev->pid,SIGUSR1);
    int fd = open(timer.dev->fifo, O_WRONLY);
    write(fd, msg, BUFFSIZE);
    close(fd);

    return 1;
}

void delDevTimer(MessageDel del_msg, char fifo[]) {
    int ack,id;
    id = del_msg.id;
    int updateParent_c; //variabile per il messaggio da inviare al figlio
    int updateParent_p; //variabile per il messaggio da inviare al padre
    int se_eliminare; //0=non terminerà, 1=terminerà alla fine
    
    
    if(timer.id == del_msg.id || del_msg.id == 0) { //trovato l'id del device
        if(del_msg.save==1){
            int id = 0;
            char file[20];
            //Se ha figli, devo richiedere anche il id del figlio
            if(timer.dev != NULL) {
                MessageCheck struct_msg = createCheck(0,0);
                char msg[BUFFSIZE];
                serializeCheck(msg,struct_msg);
                //invio il messaggio e attendo la risposta
                sendDev(msg);
                int fd = open(timer.dev->fifo,O_RDONLY);
                read(fd,msg,BUFFSIZE);
                struct_msg = deSerializeCheck(msg);
                id = struct_msg.id;
                close(fd);
            }
            sprintf(file,"%s%d.txt", PATH_FILE, timer.id);
            saveTimer(file, timer, id);
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

    MessageDel msg_c= createDel(id, del_msg.save); //creo il messaggio di delete per il figlio
    if(del_msg.save){//se i figli si devono salvare, devono anche rispondere
        updateParent_c = 1;
    }
    msg_c.updateParent=updateParent_c;
    char serialised_msg_c[BUFFSIZE]; //dim migliore?
    serializeDel(serialised_msg_c, msg_c);//serializzo il messaggio

    
    if(sendDev(serialised_msg_c)){ //Se ha un dispositivo figlio, gli manda un messaggio
        if(updateParent_c){ //se il figlio doveva rispondere, si mette in ascolto
            int pid = readDevDel(timer.dev);
            //pid=-1  ->ack negativo
            //pid=1   ->ack positivo, figlio eliminato
            if(pid != -1) {
                ack = 1;
                if(pid == 1) {
                    free(timer.dev);    //Elimino il figlio dal timer
                    timer.dev = NULL;
                    timer.type = EMPTY; //Il tipo torna vuoto
                    wait(NULL);
                }
            } else {
                ack = 2;
            }            
        }
    }
    
    if(del_msg.updateParent){ //se deve rispondere al padre
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
        exit(0);
    }
}

void infoDevTimer(MessageInfo info_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];

    if(timer.id == info_msg.id) {
        //Il controller richiede le info
        info_msg.ack = 1;           //Setto ack positivo
        info_msg.id = 0;            //Setto id=0 se è necessario avere info del figlio
        serializeInfo(serialised_msg, info_msg);
        if(sendDev(serialised_msg)) {
            //Attendo la risposta del figlio
            int fd = open(timer.dev->fifo, O_RDONLY);
            read(fd, serialised_msg, BUFFSIZE);
            close(fd);
            info_msg = deSerializeInfo(serialised_msg);
            //Se ricevo un timer o un hub non ho figli del tipo indicato da timer.type
            //Quindi restituisco solo le info del mio device
            if(!strncmp(info_msg.device, "hub", 3) || !strncmp(info_msg.device, "timer", 5)) {
                info_msg.type = TIMER;
                serializeTimer(info_msg.device, timer);
            }else {
                switch(timer.type) {
                    case BULB: {
                        Bulb bulb = deSerializeBulb(info_msg.device);
                        //Controlla se c'è override
                        if(timer.state != bulb.state)
                            bulb.state = timer.state ? 3 : 2;
                        //Crea un timer di bulb
                        TimerOfBulb tbulb = createTimerOfBulb(timer, bulb);
                        serializeTimerOfBulb(info_msg.device, tbulb);
                        info_msg.type = TIMER_OF_BULB;
                        break;
                    }
                    case WINDOW: {
                        Window window = deSerializeWindow(info_msg.device);
                        //Controlla se c'è override
                        if(timer.state != window.state)
                            window.state = timer.state ? 3 : 2;
                        //Crea un timer di window
                        TimerOfWindow twindow = createTimerOfWindow(timer, window);
                        serializeTimerOfWindow(info_msg.device, twindow);
                        info_msg.type = TIMER_OF_WINDOW;
                        break;
                    }
                    case FRIDGE: {
                        Fridge fridge = deSerializeFridge(info_msg.device);
                        //Controlla se c'è override
                        if(timer.state != fridge.state)
                            fridge.state = timer.state ? 3 : 2;
                        //Crea un timer di fridge
                        TimerOfFridge tfridge = createTimerOfFridge(timer, fridge);
                        serializeTimerOfFridge(info_msg.device, tfridge);
                        info_msg.type = TIMER_OF_FRIDGE;
                        break;
                    }
                    default: {
                        info_msg.type = TIMER;
                        serializeTimer(info_msg.device, timer);
                    }
                }
            }
        }else {
            //Se non ha figli invia solo le proprie info
            info_msg.type = TIMER;
            serializeTimer(info_msg.device, timer);
        }
        serializeInfo(serialised_msg, info_msg);
    }else if(info_msg.id == 0) {
        //Un hub richiede le info
        serializeInfo(serialised_msg, info_msg);
        if(sendDev(serialised_msg)) {
            //Attendo la risposta del figlio
            int fd = open(timer.dev->fifo, O_RDONLY);
            read(fd, serialised_msg, BUFFSIZE);
            close(fd);
            //Invio senza modificare il messaggio
        }else {
            info_msg.ack = 1;           //Setto ack positivo
            info_msg.type = TIMER;      //Indico il tipo delle info
            //Incapsulo all'interno del messaggio le info del device
            serializeTimer(info_msg.device, timer);
            serializeInfo(serialised_msg, info_msg);
        }
    }else {
        //Se il messaggio non è per questo device
        //Inoltro il messaggio ai figli se sono presenti
        serializeInfo(serialised_msg, info_msg);
        if(!sendDev(serialised_msg)) {
            info_msg.ack = 2;
            serializeInfo(serialised_msg, info_msg);
        }else {
            //Attendo la risposta del figlio
            int fd = open(timer.dev->fifo, O_RDONLY);
            read(fd, serialised_msg, BUFFSIZE);
            close(fd);
            //Invio senza modificare il messaggio
        }
    }
    //Invio al padre il messaggio
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}


void checkDevTimer(MessageCheck check_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    if(timer.id == check_msg.id || check_msg.id == 0) {
        //Trovato l'id cercato
        check_msg.ack = 1;
        check_msg.id = timer.id;
        check_msg.pid = getpid();
        check_msg.type = timer.type;
        check_msg.oldestParent = getpid();
        if(timer.dev==NULL){
            check_msg.isControl = 2;
        }else{
            check_msg.isControl = 3;
        }
        //genero il messaggio stringa
        serializeCheck(serialised_msg, check_msg);
    } else {
        //Inoltro il messaggio al figlio
        serializeCheck(serialised_msg, check_msg);
        sendDev(serialised_msg);
        //Attendo la risposta del figlio
        int fd = open(timer.dev->fifo, O_RDONLY);
        read(fd, serialised_msg, BUFFSIZE);
        close(fd);
        //invio il messaggio al padre
        check_msg = deSerializeCheck(serialised_msg);
        check_msg.oldestParent = getpid();
        if(check_msg.ack == 1 && check_msg.idAncestor == timer.id) {
            check_msg.ack = 3; //Errore nell'ancestor
        }
        serializeCheck(serialised_msg, check_msg);
    }
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}

MessageInfo readDevInfo() { 
    MessageInfo retInfo;
    retInfo.ack = 2; //Se non si trovano ack positivi viene restituito ack negativo
    if(!timer.dev){
        fprintf(stderr,"%d: Errore, il dispositivo %d non ha un dispositivo controllato\n.", getpid(), timer.id);
        return retInfo;
    }else{
        char msg[BUFFSIZE];

        int fd = open(timer.dev->fifo, O_RDONLY);
        read(fd, msg, BUFFSIZE);
        MessageInfo struct_msg = deSerializeInfo(msg);
        close(fd);
        //Se l'ack è positivo devo restituirlo perchè contiene le info del device
        if(struct_msg.ack == 1){
            retInfo = struct_msg;
        }
        return retInfo;
    }
}

int readDevDel() { 
    int pid = -1; //se l'ack è negativo
    if(!timer.dev) {
        fprintf(stderr,"%d: Errore, il dispositivo %d non ha un dispositivo controllato\n.", getpid(), timer.id);
        return pid;
    } else {
        char msg[BUFFSIZE];
        int fd = open(timer.dev->fifo, O_RDONLY);
        read(fd, msg, BUFFSIZE);
        MessageDel struct_msg = deSerializeDel(msg);
        close(fd);
        if(struct_msg.ack==1){
            pid=1;
            if(struct_msg.updateParent==0){
                pid = 0;
            }
        }      
        return pid;
    }
}

void switchDevTimer(MessageSwitch switch_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    int returnAck = 3;          //Di default l'ack ritornato al padre è negativo (id invalido / irraggiungibile)
    int returnNewState = -1;    //Di default newState ritornato è -1
    
    if(timer.id==switch_msg.id || switch_msg.id == 0){        
        unsigned int day, hours, minutes;   //Variabili in cui salvare l'input
        
        //Controlla se il messaggio è per uno dei registri del device
        if(!strncmp("begin", switch_msg.label, 5)) {
            if(!strncmp("null", switch_msg.pos, 4)) {
                //Assegna a begin il valore 0 che corrisponde a 'null'
                timer.begin = 0;
                //L'ack viene settato a positivo (nessun cambio di stato)
                returnAck = 2;
            }else if(sscanf(switch_msg.pos, "%d-%d:%d", &day, &hours, &minutes) == 3) {
                //Crea uno struct con data relativa ad adesso
                time_t newBegin = time(NULL);
                struct tm *struct_NewBegin = localtime(&newBegin);
                
                //Assegna a questa nuova data l'input del messaggio
                struct_NewBegin->tm_mday = day;
                struct_NewBegin->tm_hour = hours;
                struct_NewBegin->tm_min = minutes;
                struct_NewBegin->tm_sec = 0;
                
                //Converte lo struct in time_t
                newBegin = mktime(struct_NewBegin);

                //Assegna il nuovo valore al campo 'begin' del timer
                timer.begin = newBegin;
                //L'ack viene settato a positivo (nessun cambio di stato)
                returnAck = 2;
            }else {
                //L'ack viene settato a negativo (pos invalida)
                returnAck = 4;
            }
        }else if(!strncmp("end", switch_msg.label, 3)) {
            if(!strncmp("null", switch_msg.pos, 4)) {
                //Assegna a end il valore 0 che corrisponde a 'null'
                timer.end = 0;
                //L'ack viene settato a positivo (nessun cambio di stato)
                returnAck = 2;
            }else if(sscanf(switch_msg.pos, "%d-%d:%d", &day, &hours, &minutes) == 3) {
                //Creo uno struct con data relativa ad adesso
                time_t newEnd = time(NULL);
                struct tm *struct_NewEnd = localtime(&newEnd);
                
                //Assegno a questa nuova data l'input del messaggio
                struct_NewEnd->tm_mday = day;
                struct_NewEnd->tm_hour = hours;
                struct_NewEnd->tm_min = minutes;
                struct_NewEnd->tm_sec = 0;
                
                //Converto lo struct in time_t
                newEnd = mktime(struct_NewEnd);

                //Assegno il nuovo valore al campo 'end' del timer
                timer.end = newEnd;
                //L'ack viene settato a positivo (nessun cambio di stato)
                returnAck = 2;
            }else {
                //L'ack viene settato a negativo (pos invalida)
                returnAck = 5;
            }
        }else {
            //Se il messaggio non riguarda i registri del device
            //Invia il messaggio al figlio
            switch_msg.id = 0;

            serializeSwitch(serialised_msg, switch_msg);

            //Manda il messaggio a suo figlio
            if(!sendDev(serialised_msg)) {
                //Se non ha figli l'ack viene settato a negativo (label invalido)
                //perchè non sono presenti interruttori
                returnAck = 4;
            }else {
                //Leggo la risposta del figlio
                int fd = open(timer.dev->fifo, O_RDONLY);
                read(fd, serialised_msg, BUFFSIZE);
                MessageSwitch childSwitch = deSerializeSwitch(serialised_msg);
                close(fd);
                //L'ack da ritornare viene settato in base alla risposta del figlio
                returnAck = childSwitch.ack;
                //Se c'è stato un cambio di stato deve aggiornare lo stato dell'hub
                //con il valore all'interno dello switch
                if((childSwitch.ack == 1)  && childSwitch.newState != -1) {
                    timer.state = childSwitch.newState;
                }
                //E aggiornare il newState da ritornare
                returnNewState = childSwitch.newState;
            }
        }
    }

    //Rimando il messaggio al padre con ack e newState specificati
    switch_msg.ack = returnAck;
    switch_msg.newState = returnNewState;
    
    //Invia al padre il messaggio
    serializeSwitch(serialised_msg, switch_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}


void listDevTimer(MessageList list_msg, char fifo[]) {
    //dato il livello n, lascia n spazi prima di stamparsi
    int i;
    for(i=0;i<list_msg.level;i++){
        printf("\t");
    }
    char state[5];
    if(timer.state){
        strcpy(state,"ON");
    }else{
        strcpy(state,"OFF");
    }
    if(timer.type == -1) {
        strcpy(state,"");
    }
    printf("| -%s  %s\n",timer.name,state);
    list_msg.level++; //aumento il livello per gli eventuali figli
    char serialised_msg[BUFFSIZE];

    if(timer.dev!=NULL){//se ha un figlio, inoltra il messaggio e aspetta la risposta
        char response[BUFFSIZE];
        serializeList(serialised_msg,list_msg);
        SubDev* i = timer.dev;
        kill(i->pid,SIGUSR1);
        int fd = open(i->fifo, O_WRONLY);
        write(fd, serialised_msg, BUFFSIZE);
        close(fd);
        fd=open(i->fifo, O_RDONLY);
        read(fd,response,BUFFSIZE);
        close(fd);
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
        timer.type = tipo;
    }
    else if(pid > 0) {  //Padre, aggiorna la lista e crea la fifo
        char tmpfifo[20];
        sprintf(tmpfifo, "%s%d_%d", PATH_FIFO, getpid(), pid);
        mkfifo(tmpfifo, 0666);
        
        timer.dev=(SubDev*)malloc(sizeof(SubDev));
        strcpy(timer.dev->fifo,tmpfifo);
        timer.dev->pid=pid;
        timer.dev->next=NULL;
    }
    else {
        fprintf(stderr,"%d: Errore, impossibile aggiungere dispositivo: %s\n",getpid(),strerror(errno));
        return;
    }
}
void linkDevTimer(MessageLink link_msg, char fifo[]) {
    char serialised_msg[BUFFSIZE];
    //controllo è l'id dell'hub
    if(link_msg.id2==timer.id){
        buildTree(link_msg.id1,0);
        link_msg.ack=1;
        timer.type=link_msg.type_id1;
    }else{
        serializeLink(serialised_msg, link_msg);
        if(sendDev(serialised_msg)){//Se ha figli
            link_msg.ack=readDevLink();
        }
    }

    serializeLink(serialised_msg, link_msg);
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
}


int readDevLink() { 
    int da_ritornare = 0; //se l'ack del figlio è negativo
    if(!timer.dev) {
        return da_ritornare;
    } else {
        char msg[BUFFSIZE];
        int fd = open(timer.dev->fifo, O_RDONLY);
        read(fd, msg, BUFFSIZE);
        MessageLink link_msg = deSerializeLink(msg);
        close(fd);
        if(link_msg.ack==1){
            da_ritornare=1;
        }
    }
    return da_ritornare;
}

void updateDevTimer(MessageUpdate update_msg, char fifo[]){
    if(update_msg.ack){
        timer.type=update_msg.type;
    }else{
        update_msg.type=-1;
    }
    char serialised_msg[BUFFSIZE];
    
    serializeUpdate(serialised_msg,update_msg);
    
    //se ha un figlio, gli inoltra il messaggio
    if(timer.dev!=NULL){
        sendDev(serialised_msg);
        update_msg = readDevUpdate();
    }
    
    serializeUpdate(serialised_msg, update_msg);
    //risponde al padre
    int fd = open(fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
    
}

MessageUpdate readDevUpdate() {
    MessageUpdate da_ritornare;
    da_ritornare = createUpdate(0,0);
    da_ritornare.type = -1;
    if(timer.dev==NULL) {
        printf("Errore, nessuno da leggere.\n");
        return da_ritornare;
    } else {
        char msg[BUFFSIZE];
        SubDev* i = timer.dev;
        int fd = open(i->fifo, O_RDONLY);
        read(fd, msg, BUFFSIZE);
        MessageUpdate struct_msg = deSerializeUpdate(msg);
        close(fd);
        //Se ha un dispositivo di interazione, resituisco il msg settando il tipo trovato
        if(struct_msg.type != -1){
            da_ritornare = struct_msg;
        }
        return da_ritornare;
    }
}