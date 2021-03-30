#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "devList.h"

//Crea una nuova lista
DevList* createDevList() {
    DevList* list = (DevList*)malloc(sizeof(DevList));
    list->head = NULL;
    list->dim = 0;
    return list;
}

//Restituisce la dimensione della lista
int getDimList(const DevList* list) {
    return list->dim;
}
//Dato un pid, cerca la relativa entry e la rimuove. Restituisce 0 come conferma o -1 se il pid non esiste
int removeDevByPid(DevList* list, const pid_t pid) {
    if(getDimList(list) == 0)
        return -1;
    else {  //Caso particolare in cui il pid sta in testa
        if(list->head->pid == pid) {
            SubDev* tmp = list->head; //P. copia per l'eliminazione
            list->head = list->head->next; //La nuova testa è quella successiva
            free(tmp);  //Libero la memoria
            list->dim--;
            return 0;
        }
        else {  //Caso generico
            SubDev* prev = list->head;
            SubDev* i = prev->next;
            while(i != NULL) { //Continuo fino a fine coda
                if(i->pid == pid) {
                    prev->next = i->next;
                    free(i);
                    list->dim--;
                    return 0;
                }
                prev = i;
                i = i->next;
            }
            return -1;
        }
        return -1;
    }
}

//Dato un pid, controlla se è presente nella lista. Restituisce la relativa entry o null, se non esiste
SubDev* existsDev(const DevList* list, const pid_t pid) {
    if(getDimList(list) == 0)
        return NULL;  //Lista vuota
    else {
        SubDev* i = list->head;
        while(i != NULL) {
            if(i->pid == pid)
                return i;
            i = i->next;
        }
        return NULL;
    }
}

//Aggiunge alla lista una nuova entry, con pid e descrittore della fifo come argomenti
void addToList(DevList* list, pid_t pid, char fifo[]) {
    if(getDimList(list) == 0) {
        list->head = (SubDev*)malloc(sizeof(SubDev));
        list->head->pid = pid;
        strcpy(list->head->fifo,fifo);
        list->head->next = NULL;
    }
    else {
        SubDev* newDev = (SubDev*)malloc(sizeof(SubDev));
        strcpy(newDev->fifo,fifo);
        newDev->pid = pid;
        newDev->next = list->head;
        list->head = newDev;
    }
    list->dim++;
}

//Cancella tutta la lista
void removeAll(DevList* list) {
    if(list->dim == 1) {
        free(list->head); //Unico elemento, libero solo la coda
    }
    else if(list->dim > 1) {    //Altrimenti bisogna scorrere ed eliminarli tutti
        SubDev* prev = list->head;
        SubDev* i = prev->next;
        while(i != NULL) {
            free(prev);
            prev = i;
            i = i->next;
        }
        free(prev);
    }
    free(list); //E infine deallocare il puntatore stesso
}