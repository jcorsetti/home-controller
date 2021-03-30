#ifndef __UTILITY__
#define __UTILITY__

#define MAXARGS 4       // Numero massimo di argomenti, oltre a 4 vengono ignorati
#define BUFFSIZE 128    // Dimensione del buffer dei messaggi
#define DELIM " "       // Delimitatore stringhe

//Path degli eseguibili dei vari dispositivi

//Path per l'hub
#define SRC_HUB "./hub"
//Path per il timer
#define SRC_TIMER "./timer"
//Path per la lampadina
#define SRC_BULB "./bulb"
//Path per la finestra
#define SRC_WINDOW "./window"
//Path per il frigorifero
#define SRC_FRIDGE "./fridge"

//Cartella di creazione dei file temporanei (usata ad es. dalla buildTree)
#define PATH_FILE "src/tmp/"
//Cartella di creazione delle fifo
#define PATH_FIFO "src/tmp/"
//Cartella degli esempi disponibili
#define PATH_EXAMPLE "src/examples/"
//Fifo di comunicazione dedicata al controllo manuale
#define MANUAL_FIFO "src/tmp/manual_fifo"
//File di testo in cui la centralina scrive il proprio pid
#define CONTROLLER_TXT "src/tmp/centralina.txt"
//Conversione stringa - intero
int strtoint(char *num);

//Massimo
int max(int a, int b);

//Funzione che elimina un file, specificando il path
void deleteFile(char *file);

void printBlue(char *string);
void printYellow(char *string);
void printRed(char *string);

#endif