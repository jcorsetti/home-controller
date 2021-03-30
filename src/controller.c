#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "utility.h"
#include "messages.h"

extern int errno;

//Informazioni del controller
Controller cont;

/**
 * Manda un qualsiasi tipo di messaggio a tutti i dispositivi della sua lista
 * ritorna 1 se ha mandato i messaggi, 0 se non li ha mandati in caso di lista vuota
**/
int sendAllDev(char *msg);

//Gestisce il comando 'list'
void listDev();

//Gestisce il comando 'add'
void addDev(const int type);

/**
 * Legge solo messaggi di tipo delete:
 * ritorna una struct DelResponse contenente:
 * pid: -1 se nessun dispositivo è stato eliminato
 *       0 se è stato eliminato un dispositivo nipote della centralina
 *       x = pid del device eliminato se è stato eliminato un figlio diretto della centralina
 * *figlio: il subdevice della centralina che ha inoltrato il messaggio (serve per fare l'update dei tipi)
**/
DelResponse readAllDevDel();

/**
 * Gestisce il comando del
 * id -> id del device da eliminare
 * save -> indica se il device deve salvare le sue informazioni prima di eliminarsi
**/
void delDev(int id, int save);

/**
 * Collega id1 ad id2 se sono compatibili
**/
void linkDev(int id1, int id2);

/**
 * Legge solo messaggi di tipo switch:
 * ritorna un valore di ack:
 * 1 => ack positivo
 * 2 => ack negativo (id non esiste / irraggiungibile)
 * 3 => ack negativo (label non esiste)
 * 4 => ack negativo (pos invalida)
 * 5 => ack positivo (cambio di stato)
**/
MessageSwitch readAllDevSwitch();

//Gestisce il comando 'switch' globale
void allSwitchDev(char* pos);

//Gestisce il comando 'switch'
void switchDev(int id, char* label, char* pos);

/**
 * Legge solo messaggi di tipo info:
 * ritorna il messaggio con ack positivo (ack=1)
 * se non è presente ritorna un messaggio con ack negativo
**/
MessageInfo readAllDevInfo();

//Gestisce il comando 'info'
void infoDev(int id);

/**
 * Funzione invocata quando si effettua il linking di un dispositivo alla centralina o quando si carica un esempio. Ricostruisce l'albero dei dispositivi 
 * (o un singolo dispositivo) dal file relativo al dispositivo con id dato come parametro. Con buildFlag = 0 (utilizzato con l'operazione link) i file sono cercati
 * in PATH_FILE ed eliminati dopo la lettura. Altrimenti (utilizzato quando si lancia il controller con un esempio) i file sono cercati in PATH_EXAMPLE e sono
 * mantenuti dopo la lettura, con il numero di esempio indicato in buildFlag
*/
void buildTree(int id, int buildFlag);

/**
 * Legge solo messaggi di tipo check:
 * restituisce il messaggio Check con ack positivo se c'è, altrimenti un ack negativo
**/
MessageCheck readAllDevCheck(const DevList* list);

/**
 * Fa il check di un dispositivo dato l'id:
 * restituisce il messaggio Check con ack positivo se c'è, altrimenti un ack negativo
**/
MessageCheck checkDev(int id, int idAncestor);

/**
 *Dato un subDev, aggiorna a lui e ai suoi figli il type:
 -1 -> nessuno dispositivo di interazione tra i figli
  3 -> tra i figli ci sono solo dispositivi di interazione bulb 
  4 -> tra i figli ci sono solo dispositivi di interazione window
  5 -> tra i figli ci sono solo dispositivi di interazione fridge
**/
void updateDev(SubDev* device);

/**
 * Carica l'esempio numEx dalla cartella src/examples/
**/ 
void loadExample(int numEx);
/**
 * Handler per gestire la richiesta check del controller manuale 
**/
void handlerManuale(int sig){
    
    int mfd = open(MANUAL_FIFO, O_RDONLY);//apre la fifo dedicata al controllo manuale
    if(mfd == -1){
        fprintf(stderr,"%d: Errore nella comunicazione con il controllo manuale.\n",getpid()); //Errore con il percorso della pipe
        return;
    }
    char msg[BUFFSIZE];
    read(mfd, msg, BUFFSIZE);//legge il messaggio che sarà solamente di tipo check
    close(mfd);
    MessageCheck checkReturn;
    if(!strncmp(msg,"check", 5)){
        MessageCheck check_msg = deSerializeCheck(msg); //assumo l'ack che segna che arriva dal controllo manuale già settato
        checkReturn = checkDev(check_msg.id, 0);
    }else{
        fprintf(stderr,"%d: Errore, comando manuale non riconosciuto\n.",getpid());
        return;
    }
    serializeCheck(msg,checkReturn);
    mfd = open(MANUAL_FIFO, O_WRONLY);
    write(mfd, msg, BUFFSIZE);
    close(mfd);
}

int main(int argc, char **argv) {

    //Scrive su file il proprio pid per il controllo manuale
    FILE *stream_pid = fopen(CONTROLLER_TXT, "w");
    fprintf(stream_pid, "%d",getpid());
    fclose(stream_pid);

    char cmd[BUFFSIZE];
	char args[MAXARGS][BUFFSIZE];
    char delimiter[2] = " ";
    cont.state = 1;
    cont.mainSwitch = 1;
    cont.progId = 1;    //Il controller ha id=1
    cont.list = createDevList();

    //Stampa l'header della shell
    system("cat ./src/header.txt");
    //Gestione del caricamento degli esempi
    if(argc == 3) { //./controller -load <num_esempio>
        if(strcmp(argv[1],"-load") == 0 ) {
            int numEx;
            sscanf(argv[2],"%d",&numEx);
            loadExample(numEx);
        }
    }

    signal(SIGUSR2,handlerManuale);
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
        } else if(!strcmp(args[0], "help")) {
            system("cat ./src/help.txt");
        } else if(!strcmp(args[0], "list")) {
            listDev();
        } else if(!strcmp(args[0], "add")) {
            if(!strcmp(args[1],"hub")) {
                addDev(1);
            } else if(!strcmp(args[1],"timer")) {
                addDev(2);
            } else if(!strcmp(args[1],"bulb")) {
                addDev(3);
            } else if(!strcmp(args[1],"window")) {
                addDev(4);
            } else if(!strcmp(args[1],"fridge")) {
                addDev(5);
            } else {
                printf("Si possono aggiungere solo hub,timer,bulb,window o fridge\n");
            }

        } else if(!strcmp(args[0], "del")) {

            delDev(strtoint(args[1]), 0);

        } else if(!strcmp(args[0], "link") && !strcmp(args[2], "to")) {

            linkDev(strtoint(args[1]), strtoint(args[3]));

        } else if(!strcmp(args[0], "switch")) {

            switchDev(strtoint(args[1]), args[2], args[3]);

        } else if(!strcmp(args[0], "info")) {

            infoDev(strtoint(args[1]));

        } else if(!strcmp(args[0], "\n")) {

            //Se il comando è vuoto viene ignorato

        } else if(!strcmp(args[0], "all")) {

            allSwitchDev(args[1]);

        } else {
            printRed("ERRORE: comando non riconosciuto.\n");
        }
    }
    //Uccide il gruppo di processi se si digita quit
    kill(-(getpid()),SIGTERM);
    return 0;
}

int sendAllDev(char *msg) {
    if(getDimList(cont.list) == 0)
        return 0;  //Lista vuota
    else {
        SubDev* i = cont.list->head;
        while(i != NULL) {
            kill(i->pid,SIGUSR1); //Il processo a cui comunicare viene svegliato con un segnale
            int fd = open(i->fifo, O_WRONLY);
            write(fd, msg, BUFFSIZE);   //E gli viene inviato il messaggio
            close(fd);
            i = i->next;
        }
        return 1;
    }
}

// LIST

void listDev() {
	printf("Controller_001  \n");

    if(getDimList(cont.list) == 0){
        return;  //Lista vuota
    }
    //Crea il messaggio list e lo serializza
    MessageList msg = createList();
    char serialised_msg[BUFFSIZE];
    char response[BUFFSIZE];
    serializeList(serialised_msg,msg);
    //Per ogni figlio manda il messaggio e attende subito una risposta
    SubDev* i = cont.list->head;
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

// ADD

void addDev(const int type) {
    cont.progId++;      //Id progressivo
	char src[20];       //Nome del file da eseguire
    switch(type) {
        case 1: strcpy(src,SRC_HUB);
        break;
        case 2: strcpy(src,SRC_TIMER);
        break;
        case 3: strcpy(src,SRC_BULB);
        break;
        case 4: strcpy(src,SRC_WINDOW);
        break;
        case 5: strcpy(src,SRC_FRIDGE);
        break;
        default: {  //Questo non dovrebbe mai succedere, gli argomenti sono controllati prima
            fprintf(stderr,"%d: Errore nella creazione di un nuovo dispositivo.\n",getpid());
            return;        
        }
    }

    int pid = fork();
    if(pid == 0) {  //Il figlio esegue l'exec del processo relativo al dispositivo da creare
        char stringid[10];
        sprintf(stringid, "%d",cont.progId);
        char* argg[]={src, stringid, NULL};
        execv(argg[0],argg);    

    } else if(pid > 0) {    //il padre crea la fifo
        char tmpfifo[20];
        sprintf(tmpfifo, "%s%d_%d", PATH_FIFO, getpid(), pid);
        mkfifo(tmpfifo, 0666); //Non si sa mai l'ordine di esecuzione
        switch(type) {
            case HUB: printf("Aggiunto hub con id %d\n",cont.progId);
            break;
            case TIMER: printf("Aggiunto timer con id %d\n",cont.progId);
            break;
            case BULB: printf("Aggiunto bulb con id %d\n",cont.progId);
            break;
            case WINDOW: printf("Aggiunto window con id %d\n",cont.progId);
            break;
            case FRIDGE: printf("Aggiunto fridge con id %d\n",cont.progId);
            break;
        }
        addToList(cont.list, pid, tmpfifo);
        
    } else {
        fprintf(stderr,"%d: Errore nella creazione di un nuovo dispositivo.\n",getpid());
        return;
    }
}

// DEL

void delDev(int id, int save) {
    if(id<0){
        printf("ID non valido\n");
        return;
    }
    if(id==1){
        printf("Impossibile eliminare la centralina\n");
        return;
    } 
    MessageDel msg= createDel(id, save); //creo il messaggio di delete da inviare ai figli
    char serialised_msg[BUFFSIZE]; //dim migliore?
    serializeDel(serialised_msg,msg);//trasformo il messaggio in stringa
    
    //Manda il messaggio a tutti i suoi figli
    if(!sendAllDev(serialised_msg)){ //Se ha dispositivi nella lista manda i messaggi, altrimenti si ferma
        printf("Non ci sono dispositivi collegati\n");
        return;
    }
    //legge gli ack dei figli
    DelResponse risposta=readAllDevDel(cont.list);
    int pid_risposta;
    //pid=-1  ->tutti gli ack sono negativi
    //pid=0   ->c'è un ack positivo ma con updateParent=0
    //pid=id del processo da togliere della lista ->ack positivo con updateParent=1
    if(risposta.pid!=-1){//ack positivo, un dispositivo è stato eliminato
        if(risposta.pid!=0){//updateParent=1, il dispositivo eliminato era un figlio, devo toglierlo dalla lista
            pid_risposta= risposta.figlio->pid;
            removeDevByPid(cont.list,risposta.pid);
            wait(NULL);//elimina il processo defunct (rimane zoombie se non chiamo la wait)
        }
        if(!save){
            printf("Dispositivo con ID %d eliminato\n",id);
        }


        if(pid_risposta!=risposta.pid){
            updateDev(risposta.figlio);
        }
        return;
    }
    //Se tutti gli ack ricevuti sono negativi, nessun dispositivo è stato eliminato
    printf("Nessun dispositivo con ID %d collegato!\n",id);
}

DelResponse readAllDevDel() { 
    DelResponse da_ritornare;
    da_ritornare.pid=-1; //id del processo da ritornare, se tutti gli ack sono negativi, ritorna 0
    if(getDimList(cont.list) == 0){
        fprintf(stderr,"%d: Errore, la centralina non ha dispositivi controllati\n.", getpid());
        return da_ritornare;
    }else{
        char msg[BUFFSIZE];
        SubDev* i = cont.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageDel struct_msg = deSerializeDel(msg);
            close(fd);
            
            if(struct_msg.ack==1){
                da_ritornare.pid=struct_msg.id;
                da_ritornare.figlio=i;
                if(struct_msg.updateParent==0){
                    da_ritornare.pid=0;
                }
            }   
            //anche se ha gia trovato un dispositivo con ack positivo, deve cmq leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
        }
        return da_ritornare;
    }
}

// LINK
void linkDev(int id1, int id2) {
    //se i due dispositivi coincidono
    if(id1 == id2) {
        printf("I due dispositivi coincidono.\n");
        return;
    }
    //controllo i due id
    MessageCheck check1 = checkDev(id1, 0);
    //se id2 è della centralina
    if(id2==1){
        if(check1.ack==1){
            delDev(id1, 1);
            //usleep(500000);
            buildTree(id1,0);
            printf("%d e' stato collegato a %d\n",id1, id2);
            return;
        } else {
            printf("Id %d non esiste\n", id1);
            return;
        }
    }
    MessageCheck check2 = checkDev(id2, id1);  
    if(check2.ack == 3) { //Errore di ancestor
        printf("Non è possibile collegare un dispositivo ad un suo dispositivo controllato\n");
        return;
    }
    //se esistono i due id
    if(check1.ack!=1 || check2.ack!=1){
        printf("Almeno uno dei dispositivi non esiste\n");
        return;
    }

    //se il secondo id è di un dispositivo di controllo
    if(!check2.isControl){
        printf("Il secondo dispositivo non è di controllo\n");
        return;
    }
    //se il secondo dispositivo è un timer già con un figlio
    if(check2.isControl==3){
        printf("Il timer ha già un dispositivo collegato!\n");
        return;
    }
    //se i due id sono compatibili
    if(!(check1.type==check2.type || check2.type==-1 || check1.type==-1)){
        printf("I due dispositivi non sono compatibili\n");
        return;
    }
    
    //elimino e salvo il primo dispositivo
    delDev(id1, 1);
    //creo il messaggio link
    MessageLink link_msg = createLink(id1, id2, check1.type);
    char serialised_msg[BUFFSIZE];
    serializeLink(serialised_msg,link_msg);

   //mando direttamente il messaggio di link al padre piu vecchio di id2
    SubDev *device = existsDev(cont.list, check2.oldestParent);
    kill(check2.oldestParent,SIGUSR1);
    int fd = open(device->fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
    
    //leggo il messaggio
    fd = open(device->fifo, O_RDONLY); 
    read(fd, serialised_msg, BUFFSIZE);
    link_msg = deSerializeLink(serialised_msg);
    close(fd);

    if(link_msg.ack) {
        printf("%d e' stato collegato a %d\n",id1, id2);
        usleep(500000);
        //aggiorna il tipo di dispositivi
        updateDev(device);
        return;
    } else {
        fprintf(stderr,"%d: Errore nel collegamento dei dispositivi.\n",getpid());
        return;
    }
}

// SWITCH

void switchDev(int id, char* label, char* pos) {
	if(id<=0){
        printf("ID non valido\n");
        return;
    }
    //Il label 'perc' non puo essere modificato dal controller
    if(!strncmp(label, "perc", 4)) {
        printf("Label non modificabile.\n");
        return;
    }
    //E' possibile chiamare l'interruttore generale con il comando switch
    if(id==1 && !strncmp(label, "all", 3)) {
        allSwitchDev(pos);
        return;
    }
    //Creo il messaggio di tipo switch
    MessageSwitch msg = createSwitch(id, label, pos);
    char serialised_msg[BUFFSIZE];
    serializeSwitch(serialised_msg, msg); //Trasformo il messaggio in stringa
    //Manda il messaggio a tutti i suoi figli
    if(!sendAllDev(serialised_msg)) {
        printf("Non ci sono dispositivi collegati\n");
        return;
    }
    MessageSwitch retSwitch = readAllDevSwitch(cont.list);
    switch(retSwitch.ack) {
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
        default: printf("Ack dal comando 'switch' non riconosciuto. (%d)\n", retSwitch.ack);
    }
    
}

void allSwitchDev(char* pos) {
    char serialised_msg[BUFFSIZE];
    MessageSwitch msg;
    
    //La posizione può essere solo 'on' oppure 'off'
    if(strncmp(pos, "on", 2) && strncmp(pos, "off", 3)) {
        fprintf(stderr, "Errore, il comando all accetta solo 'on' e 'off'.");
        return;
    }
    //Creo il messaggio di tipo switch con id=0
    msg = createSwitch(0, "allSwitch", pos);
    
    serializeSwitch(serialised_msg, msg); //Trasformo il messaggio in stringa
    //Manda il messaggio a tutti i suoi figli
    if(!sendAllDev(serialised_msg)) {
        printf("Non ci sono dispositivi collegati.\n");
        return;
    }
    MessageSwitch retSwitch = readAllDevSwitch(cont.list);
    switch(retSwitch.ack) {
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
        default: printf("Ack dal comando 'switch' non riconosciuto. (%d)\n", retSwitch.ack);
    }
}

MessageSwitch readAllDevSwitch() { 
    MessageSwitch retSwitch;    //Messaggio da ritornare
    retSwitch.ack = 3;          //Se non si trovano ack positivi viene restituito il valore 3 (ack negativo id non esiste / irraggiungibile)
    
    if(getDimList(cont.list) == 0){
        fprintf(stderr,"%d: Errore, la centralina non ha dispositivi controllati\n.", getpid());
        return retSwitch;
    }else{
        char msg[BUFFSIZE];
        SubDev* i = cont.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageSwitch struct_msg = deSerializeSwitch(msg);
            close(fd);
            //Se l'ack non è negativo (id non esiste) lo assegno come valore di ritorno
            //perchè contiene ack positivo ho informazioni sul tipo di errore
            if(struct_msg.ack != 3){
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

void infoDev(int id) {
	if(id<=0){
        printf("ID non valido\n");
        return;
    }else if(id == 1) {
        printController(cont);
        return;
    }
    //Creo il messaggio di tipo info
    MessageInfo msg = createInfo(id);
    char serialised_msg[BUFFSIZE];
    serializeInfo(serialised_msg, msg); //Trasformo il messaggio in stringa
    
    //Manda il messaggio a tutti i suoi figli
    if(!sendAllDev(serialised_msg)) {
        printf("Non ci sono dispositivi collegati\n");
        return;
    }
    //Cerco il messaggio info con ack positivo
    MessageInfo retInfo = readAllDevInfo(cont.list);

    //Se non esiste lo notifico
    if(retInfo.ack == 2) {
        printf("L'id non esiste.\n");
        return;
    }
    //Stampo le informazioni in base al tipo indicato
    switch (retInfo.type) {
        case HUB: {
            Hub hub = deSerializeHub(retInfo.device);
            printHub(hub);
            break;
        }case TIMER: {
            Timer timer = deSerializeTimer(retInfo.device);
            printTimer(timer);
            break;
        }
        case BULB: {
            Bulb bulb = deSerializeBulb(retInfo.device);
            printBulb(bulb);
            break;
        }
        case WINDOW: {
            Window window = deSerializeWindow(retInfo.device);
            printWindow(window);
            break;
        }
        case FRIDGE: {
            Fridge fridge = deSerializeFridge(retInfo.device);
            printFridge(fridge);
            break;
        }case TIMER_OF_BULB: {
            TimerOfBulb tbulb = deSerializeTimerOfBulb(retInfo.device);
            printTimerOfBulb(tbulb);
            break;
        }case TIMER_OF_WINDOW: {
            TimerOfWindow twindow = deSerializeTimerOfWindow(retInfo.device);
            printTimerOfWindow(twindow);
            break;
        }case TIMER_OF_FRIDGE: {
            TimerOfFridge tfridge = deSerializeTimerOfFridge(retInfo.device);
            printTimerOfFridge(tfridge);
            break;
        }
        default: {
            printf("INFO: Device non riconosciuto. (%d)\n", retInfo.type);
        }
    }
}

MessageInfo readAllDevInfo() { 
    MessageInfo retInfo;
    retInfo.ack = 2; //Se non si trovano ack positivi viene restituito ack negativo
    if(getDimList(cont.list) == 0){
        fprintf(stderr,"%d: Errore, la centralina non ha dispositivi controllati\n.", getpid());
        return retInfo;
    }else{
        char msg[BUFFSIZE];
        SubDev* i = cont.list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageInfo struct_msg = deSerializeInfo(msg);
            close(fd);
            //Se l'ack è positivo devo restituirlo perchè contiene le info del device
            if(struct_msg.ack == 1){
                retInfo = struct_msg;
            }
            //Anche se ha gia trovato un dispositivo con ack positivo, deve comunque leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
        }
        return retInfo;
    }
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
        deleteFile(path);   //Il file viene eliminato solo se non si carica da un esempio, ma da un file temporaneo precedentemente creato

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
    }
    else if(pid > 0) {  //Padre, aggiorna la lista e crea la fifo
        char tmpfifo[20];
        sprintf(tmpfifo, "%s%d_%d", PATH_FIFO, getpid(), pid);
        mkfifo(tmpfifo, 0666);
        addToList(cont.list, pid, tmpfifo);
    }
    else {
        fprintf(stderr,"%d: impossibile aggiungere dispositivo: %s\n",getpid(),strerror(errno));
        return;
    }
}

MessageCheck checkDev(int id, int idAncestor) {
    MessageCheck check_msg = createCheck(id, idAncestor);
    char serialised_msg[BUFFSIZE];
    serializeCheck(serialised_msg, check_msg);
    //invio il messaggio ai figli
    sendAllDev(serialised_msg);
    //ricevo risposta dai figli ed inoltro al parent
    check_msg = readAllDevCheck(cont.list);
    return check_msg;
}

MessageCheck readAllDevCheck(const DevList* list) {
    MessageCheck retInfo;
    retInfo.ack = 2; //Se non si trovano ack positivi viene restituito ack negativo
    if(getDimList(list) == 0) {
        //Ritorna ack negativo se non ci sono figli
        return retInfo;
    } else {
        char msg[BUFFSIZE];
        SubDev* i = list->head;
        while(i != NULL) {
            int fd = open(i->fifo, O_RDONLY);
            read(fd, msg, BUFFSIZE);
            MessageCheck struct_msg = deSerializeCheck(msg);
            close(fd);
            //Se l'ack è positivo devo restituirlo perchè contiene le info del device
            if(struct_msg.ack == 1 || struct_msg.ack == 3)
                retInfo = struct_msg;
            //Anche se ha già trovato un dispositivo con ack positivo, deve comunque leggere 
            //dalla pipe degli altri per sbloccarli
            i = i->next;
        }
        return retInfo;
    }
}


void updateDev(SubDev *device){
    //creo il messaggio
    MessageUpdate update_msg = createUpdate(0, 0);
    char serialised_msg[BUFFSIZE];
    serializeUpdate(serialised_msg, update_msg);
    
    //mando il messaggio al figlio per scoprire il type del gruppo a cui appartiene
    //il type può solo essere -1(nessun dispositivo (di interazione), 3(solo bulb), 4(solo window), 5(solo fridge)
    kill(device->pid, SIGUSR1);
    int fd = open(device->fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
    
    //leggo il messaggio
    fd = open(device->fifo, O_RDONLY);
    read(fd, serialised_msg, BUFFSIZE);
    close(fd);
    update_msg=deSerializeUpdate(serialised_msg);
    
    //manda un nuovo messaggio ai figli con ack positivo in modo che tutti i figli settano lo stesso type
    update_msg.ack=1;
    serializeUpdate(serialised_msg,update_msg);
    kill(device->pid, SIGUSR1);
    fd = open(device->fifo, O_WRONLY);
    write(fd, serialised_msg, BUFFSIZE);
    close(fd);
    
    //leggo dal figlio per sapere quando hanno finito di settare il type
    fd = open(device->fifo, O_RDONLY); 
    read(fd, serialised_msg, BUFFSIZE);
    close(fd);
    
}

void loadExample(int numEx) {
    char path[NAMESIZE];
    sprintf(path,"%sex%d/start.txt", PATH_EXAMPLE, numEx);  //Creo il path a partire dal numero dell'esempio da caricare
    FILE *fp;
    fp = fopen(path,"r");           
    if(!fp) {
        printf("Non è stato possibile caricare l'esempio %d: %s\n",numEx,strerror(errno));
        printf("Il programma procederà con la configurazione vuota standard.\n");
        return;
    }
    else {
        int numDev;     // Numero di dispotivi caricati, necessario per settare l'id progressivo iniziale
        int numFiles;   // Numero di file radice (dispositivi direttamente collegati alla centralina)
        int rootId;     // Id di ogni radice
        int i;
        fscanf(fp,"%d %d\n",&numDev,&numFiles);
        cont.progId = numDev;
        for(i = 0; i < numFiles; i++) { //Per ogni figlio diretto della centralina, invoco la buildTree
            fscanf(fp,"%d.txt\n",&rootId);
            buildTree(rootId,numEx);
        }
        printf("E' stato caricato l'esempio %d.\n",numEx);
    }
    fclose(fp);
}