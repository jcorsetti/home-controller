#ifndef __DEVLIST__
#define __DEVLIST__

#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>

//Struttura utilizzata da un device di controllo per memorizzare le informazioni di uno (timer) o più (hub e centralina) dispositivi controllati
typedef struct SubDev_t {
    //Pid del processo
    pid_t pid;
    //Nome della FIFO per la comunicazione
    char fifo[20];
    //Puntatore al prossimo nodo
    struct SubDev_t* next;
} SubDev;

//Lista dei dispositivi figli di un dispositivo
typedef struct DevList_t {
    //Punta all'ultimo elemento inserito
    SubDev* head;
    //Dimensione attuale
    int dim;
} DevList;

//Crea una nuova lista
DevList* createDevList();
//Restituisce la dimensione della lista
int getDimList(const DevList*);
//Dato un pid, cerca la relativa entry e la rimuove. Restituisce 0 come conferma o -1 se il pid non esiste
int removeDevByPid(DevList*, const pid_t);
//Dato un pid, controlla se è presente nella lista. Restituisce la relativa entry o null, se non esiste
SubDev* existsDev(const DevList*, const pid_t);
//Aggiunge alla lista una nuova entry, con pid e descrittore della fifo come argomenti
void addToList(DevList*,pid_t,char[]);
//Cancella tutta la lista
void removeAll(DevList*);
#endif



