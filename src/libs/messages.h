#ifndef __MESSAGES__
#define __MESSAGES__

#include "devices.h"

//Dimensione massima del campo info all'interno di un messaggio 'info'
#define DEVICESIZE 128

//Dimensione massima del campo label all'interno di un messaggio 'switch'
#define LABELSIZE 16

//Dimensione massima del campo pos all'interno di un messaggio 'switch'
#define POSSIZE 16

//Dimensione massima di una data in forma di stringa
#define DATESIZE 32

//Descrive un messaggio di tipo check
typedef struct MessageCheck_t {
    /**
     * Valori possibili:
     * 0 => non è un ack
     * 1 => ack
     * 2 => nack (dispositivo non trovato)
     * 3 => nack (errore di ancestor)
     */
    int ack;
    //Identificatore del dispositivo
    int id;
    /**
     * Id del primo argomento della link, utilizzato per controllare che un antenato non venga
     * collegato ad un dispositivo nel suo sottoalbero
     */
    int idAncestor;
    /**
     * Indica il tipo di device
     * Valori possibili:
     * 1 => hub
     * 2 => timer
     * 3 => bulb
     * 4 => window
     * 5 => fridge
     */
    int type;
    //Indica il pid del processo del device
    int pid;
    //Indica se il dispositvo è hub (1), timer vuoto(2), timer con un dispositivo(3), non di controllo(0)
    int isControl;
    //Indica l'antenato piu vecchio (esclusa la centralina), serve per la link
    int oldestParent;
} MessageCheck;

//Descrive un messaggio di tipo del
typedef struct MessageDel_t {
    /**
     * Valori possibili:
     * 0 => non è un ack
     * 1 => ack
     * 2 => nack
     */
    int ack;
    //Identificatore del dispositivo
    int id;
    /**
     * Flag che indica se il device deve
     * aggiornare la sua lista di fifo.
     * Valori possibili:
     * 0 => non è un ack
     * 1 => ack
     * 2 => nack
     */
    int updateParent;
    /**
     * Flag che indica se prima di eliminarsi
     * deve salvare i propri dati su file.
     * Valori possibili:
     * 0 => non devo salvare
     * 1 => devo salvare
     */
    int save;
} MessageDel;

//Descrive un messaggio di tipo info
typedef struct MessageInfo_t {
    /**
     * Valori possibili:
     * 0 => non è un ack
     * 1 => ack
     * 2 => nack
     */
    int ack;
    //Identificatore del dispositivo
    int id;
    /**
     * Indica il tipo di device
     * Valori possibili:
     * 1 => hub
     * 2 => timer
     * 3 => bulb
     * 4 => window
     * 5 => fridge
     */
    int type;
    /**
     * Stringa contenente le informazioni
     * del device indicato dall'id.
     * E' presente solo solo se il valore di ack
     * è settato a 1.
     */
    char device[DEVICESIZE];
} MessageInfo;

//Descrive un messaggio di tipo switch
typedef struct MessageSwitch_t {
    /**
     * Valori possibili:
     * 0 => non è un ack
     * 1 => ack
     * 2 => nack (non esiste l'id specificato o non è raggiungibile)
     * 3 => nack (non esiste l'interruttore specificato)
     * 4 => nack (posizione dell'interruttore non valida)
     */
    int ack;
    //Identificatore del dispositivo
    int id;
    //Indica il nome dell'interruttore con cui interagire
    char label[LABELSIZE];
    //Indica il nuovo valore da assegnare all'interruttore
    char pos[POSSIZE];
    /**
     * Se lo switch ha provocato un cambio di stato indica il valore del nuovo
     * stato per poter aggionare anche quello dei device di controllo nei
     * livelli superiori.
    **/
    int newState;
} MessageSwitch;

//Descrive un messaggio di tipo list
typedef struct MessageList_t {
    /**
     * Valori possibili:
     * 0 => non è un ack
     * 1 => ack
     * 2 => nack
     */
    int ack;
    /**
     * Indica il livello in cui ci si trova all'interno della gerarchia
     * (il controller si trova al livello 0)
     */
    int level;
} MessageList;

//Descrive un messaggio di tipo link
typedef struct MessageLink_t {
    /**
     * Valori possibili:
     * 0 => non è un ack
     * 1 => ack
     * 2 => nack
     */
    int ack;
    //Indica l'id del device da spostare insieme agli eventuali figli
    int id1;
    //Indica l'id del device a cui collegare quello indicato dall'id id1
    int id2;
    //indica il tipo del dispositivo da linkare
    int type_id1;
} MessageLink;

//Descrive un messaggio di tipo update
typedef struct MessageUpdate_t {
    /**
     * Valori possibili:
     * 0 => da parent a figlio: indica che i figli devono controllare e mandare se ce un tipo di dispositivo di interazione
     *   => da figlio a parent: non c'è nessuno dispositivo di interazione
     * 1 => da partent a figlio: il figlio deve fare l'update del type con il valore passato
     *   => da figlio a parent: c'è almeno un dispositivo di interazione e corrisponde a type
     */
    int ack;
    //Indica il tipo di dispositivo di interazione
    int type;
} MessageUpdate;

void stateToStr(char* string, const int state);
void timeToStr(char* string, const time_t _time);

// CHECK

//Crea un messaggio di tipo check con parametri di default:
//(ack=0, type=-1, pid=-1, isControl=-1)
MessageCheck createCheck(const int id, const int idAncestor);
//Converte un messaggio di tipo check in una stringa
void serializeCheck(char* string, const MessageCheck msg);
//Converte una stringa in un messaggio check
//@return nuovo messaggio di tipo check
MessageCheck deSerializeCheck(const char* string);
//Stampa un messaggio di tipo check
void printCheck(const MessageCheck msg);

// DEL

//Crea un messaggio di tipo del con parametri di default:
//(ack=0, updateParent=1)
MessageDel createDel(const int id, const int save);
//Converte un messaggio di tipo del in una stringa
void serializeDel(char* string, const MessageDel msg);
//Converte una stringa in un messaggio del
//@return nuovo messaggio di tipo del
MessageDel deSerializeDel(const char* string);
//Stampa un messaggio di tipo del
void printDel(const MessageDel msg);

// INFO

//Crea un messaggio di tipo info con parametri di default:
//(ack=0, type=-1, info=null)
MessageInfo createInfo(const int id);
//Converte un messaggio di tipo info in una stringa
void serializeInfo(char* string, const MessageInfo msg);
//Converte una stringa in un messaggio info
//@return nuovo messaggio di tipo info
MessageInfo deSerializeInfo(const char* string);
//Stampa un messaggio di tipo info
void printInfo(const MessageInfo msg);

// SWITCH

//Crea un messaggio di tipo switch con parametri di default:
//(ack=0)
MessageSwitch createSwitch(const int id, const char* label, const char* pos);
//Converte un messaggio di tipo switch in una stringa
void serializeSwitch(char* string, const MessageSwitch msg);
//Converte una stringa in un messaggio switch
//@return nuovo messaggio di tipo switch
MessageSwitch deSerializeSwitch(const char* string);
//Stampa un messaggio di tipo switch
void printSwitch(const MessageSwitch msg);

// LIST

//Crea un messaggio di tipo list con parametri di default:
//(ack=0, level=1)
MessageList createList();
//Converte un messaggio di tipo list in una stringa
void serializeList(char* string, const MessageList msg);
//Converte una stringa in un messaggio list
//@return nuovo messaggio di tipo list
MessageList deSerializeList(const char* string);
//Stampa un messaggio di tipo list
void printList(const MessageList msg);

// LINK

//Crea un messaggio di tipo link con parametri di default:
//(ack=0)
MessageLink createLink(int id1, int id2, int type_id1);
//Converte un messaggio di tipo link in una stringa
void serializeLink(char* string, const MessageLink msg);
//Converte una stringa in un messaggio link
//@return nuovo messaggio di tipo link
MessageLink deSerializeLink(const char* string);
//Stampa un messaggio di tipo link
void printlink(const MessageLink msg);


// UPDATE

//Crea un messaggio di tipo update
MessageUpdate createUpdate(int ack, int type);
//Converte un messaggio di tipo update in una stringa
void serializeUpdate(char* string, const MessageUpdate msg);
//Converte una stringa in un messaggio update
//@return nuovo messaggio di tipo update
MessageUpdate deSerializeUpdate(const char* string);
//Stampa un messaggio di tipo update
void printUpdate(const MessageUpdate msg);

// CONTROLLER

//Stampa un controller (escluso il progId)
void printController(const Controller cont);

// HUB

//Converte un hub in una stringa
void serializeHub(char* string, const Hub hub);
//Converte una stringa in un hub
//@return nuovo hub
Hub deSerializeHub(const char* string);
//Stampa un hub
void printHub(const Hub hub);

// TIMER

//Converte un timer in una stringa
void serializeTimer(char* string, const Timer timer);
//Converte una stringa in un timer
//@return nuovo timer
Timer deSerializeTimer(const char* string);
//Stampa un timer
void printTimer(const Timer timer);

// BULB

//Converte un bulb in una stringa
void serializeBulb(char* string, const Bulb bulb);
//Converte una stringa in un bulb
//@return nuovo bulb
Bulb deSerializeBulb(const char* string);
//Stampa un bulb
void printBulb(const Bulb bulb);

// WINDOW

//Converte una window in una stringa
void serializeWindow(char* string, const Window window);
//Converte una stringa in una window
//@return nuovoa window
Window deSerializeWindow(const char* string);
//Stampa una window
void printWindow(const Window window);

// FRIDGE

//Converte un fridge in una stringa
void serializeFridge(char* string, const Fridge fridge);
//Converte una stringa in un fridge
//@return nuovo fridge
Fridge deSerializeFridge(const char* string);
//Stampa un fridge
void printFridge(const Fridge fridge);

// TIMER OF BULB

//Crea un TimerOfBulb da un timer ed un bulb
TimerOfBulb createTimerOfBulb(const Timer timer, const Bulb bulb);
//Converte un bulb in una stringa
void serializeTimerOfBulb(char* string, const TimerOfBulb bulb);
//Converte una stringa in un bulb
//@return nuovo bulb
TimerOfBulb deSerializeTimerOfBulb(const char* string);
//Stampa un bulb
void printTimerOfBulb(const TimerOfBulb bulb);

// TIMER OF WINDOW

//Crea un TimerOfWindow da un timer ed una window
TimerOfWindow createTimerOfWindow(const Timer timer, const Window window);
//Converte una window in una stringa
void serializeTimerOfWindow(char* string, const TimerOfWindow window);
//Converte una stringa in una window
//@return nuovoa window
TimerOfWindow deSerializeTimerOfWindow(const char* string);
//Stampa una window
void printTimerOfWindow(const TimerOfWindow window);

// TIMER OF FRIDGE

//Crea un TimerOfFridge da un timer ed un fridge
TimerOfFridge createTimerOfFridge(const Timer timer, const Fridge fridge);
//Converte un fridge in una stringa
void serializeTimerOfFridge(char* string, const TimerOfFridge fridge);
//Converte una stringa in un fridge
//@return nuovo fridge
TimerOfFridge deSerializeTimerOfFridge(const char* string);
//Stampa un fridge
void printTimerOfFridge(const TimerOfFridge fridge);

//struct usata pe la risposta della del del controllore
typedef struct DelResponse_t {
    //pid del device eliminato
    int pid;
    //device che ha mandato il messaggio alla centralina
    SubDev *figlio;
} DelResponse;

#endif