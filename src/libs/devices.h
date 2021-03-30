#ifndef __DEVICE__
#define __DEVICE__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "devList.h"

#define EMPTY -1 //Usato per indicare che un dispositivo di controllo non ha device collegati
#define CONTROLLER 0
#define HUB 1
#define TIMER 2
#define BULB 3
#define WINDOW 4
#define FRIDGE 5
#define TIMER_OF_BULB 6
#define TIMER_OF_WINDOW 7
#define TIMER_OF_FRIDGE 8
//Dimensione massima del nome
#define NAMESIZE 32

//Descrive il controller
typedef struct Controller_t {
    //Id progressivo assegnato ai dispositivi creati
    int progId;
    //Stato 1-On / 0-Off
    int state;
    //Interruttore generale della centralina, 1-On / 0-Off
    int mainSwitch;
    //Lista dei dispositivi controllati
    DevList* list;
} Controller;

//Descrive un dispositivo hub
typedef struct Hub_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Lista dei dispositivi controllati
    DevList* list;
    //Tipo dei dispositivi controllati
    int type;
} Hub;

//Descrive un dispositivo timer
typedef struct Timer_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Contiene i dati dell'eventuale dispositivo controllato, o NULL
    SubDev* dev;
    //Tipo del dispositivo controllato
    int type;
    //Tempo di accensione
    time_t begin;
    //Tempo di spegnimento
    time_t end;
} Timer;

//Descrive un dispositivo lampadina
typedef struct Bulb_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Interruttore 1-On / 0-Off
    int mainSwitch;
    //Tempo di attivazione
    time_t activeTime;
} Bulb;

//Descrive un dispositivo finestra
typedef struct Window_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Interruttore per aprire
    int openSwitch;
    //Interruttore per chiudere
    int closeSwitch;
    //Tempo di apertura
    time_t openTime;
} Window;

//Descrive un dispositivo frigorifero
typedef struct Fridge_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Interruttore per apertura/chiusura
    int mainSwitch;
    //Interruttore termostato, ha il valore della temperatura desiderata
    int thermostat;
    //Tempo di apertura
    time_t openTime;
    //Tempo che trascorre fra l'apertura e la chiusura automatica
    time_t delay;
    //Percentuale di riempimento
    int perc;
    //Temperatura in °C
    int temp;
} Fridge;

//Descrive un timer di lampadina
typedef struct TimerOfBulb_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Interruttore 1-On / 0-Off
    int mainSwitch;
    //Tempo di attivazione
    time_t activeTime;
    //Tempo di accensione
    time_t begin;
    //Tempo di spegnimento
    time_t end;
} TimerOfBulb;

//Descrive un timer di finestra
typedef struct TimerOfWindow_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Interruttore per aprire
    int openSwitch;
    //Interruttore per chiudere
    int closeSwitch;
    //Tempo di apertura
    time_t openTime;
    //Tempo di accensione
    time_t begin;
    //Tempo di spegnimento
    time_t end;
} TimerOfWindow;

//Descrive un timer di frigorifero
typedef struct TimerOfFridge_t {
    //Identificatore del dispositivo
    int id;
    //Nome del dispositivo
    char name[NAMESIZE];
    //Stato 1-On / 0-Off
    int state;
    //Interruttore per apertura/chiusura
    int mainSwitch;
    //Interruttore termostato, ha il valore della temperatura desiderata
    int thermostat;
    //Tempo di apertura
    time_t openTime;
    //Tempo che trascorre fra l'apertura e la chiusura automatica
    time_t delay;
    //Percentuale di riempimento
    int perc;
    //Temperatura in °C
    int temp;
    //Tempo di accensione
    time_t begin;
    //Tempo di spegnimento
    time_t end;
} TimerOfFridge;

// Salva l'hub nel file specificato
void saveHub(const char[], const Hub, const int[]);

// Salva il timer nel file specificato
void saveTimer(const char[], const Timer, const int);

// Salva la lampadina nel file specificato
void saveBulb(const char[], const Bulb);

// Salva la finestra nel file specificato
void saveWindow(const char[], const Window);

// Salva il frigorifero nel file specificato
void saveFridge(const char[], const Fridge);

//Carica un dispositivo da un file specificato
char** loadDevice(const char[]);


/** Le seguenti funzioni sono invocate dalle buildTree in Hub,Timer e Controller per la creazione del dispositivo figlio desiderato a partire da un file
*   Fields contiene i campi informativi. Il campo flag, presente solo nei dispositivi di controllo, serve ad indicare alla buildTree dei possibili figli 
*   se si vuole mantenere (in caso di lettura da un esempio) o eliminare (a seguito di una link) il file da cui è stato letto il dispositivo
*/
//Crea un hub
void makeHub(char** fields, const int flag);

//Crea un timer
void makeTimer(char** fields, const int flag);

//Crea una lampadina
void makeBulb(char** fields);

//Crea una finestra
void makeWindow(char** fields);

//Crea un frigorifero
void makeFridge(char** fields);

#endif