#include <stdio.h>
#include <stdlib.h>
#include "utility.h"

int strtoint(char *num) {
    int n = 0, i = 0, err = 0;
    while(num[i] != 0 && !err)
	{
        switch (num[i++])
        {
            case '0': n = n * 10 + 0; break;
            case '1': n = n * 10 + 1; break;
            case '2': n = n * 10 + 2; break;
            case '3': n = n * 10 + 3; break;
            case '4': n = n * 10 + 4; break;
            case '5': n = n * 10 + 5; break;
            case '6': n = n * 10 + 6; break;
            case '7': n = n * 10 + 7; break;
            case '8': n = n * 10 + 8; break;
            case '9': n = n * 10 + 9; break;
            default: n = -1; err = 1; break;
        }
    }
    return n;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

void deleteFile(char *file) {
    char cmd[BUFFSIZE];
    sprintf(cmd,"rm %s",file);
    system(cmd);
}

/*
blue:    \033[1;34m
green:   \033[0;32m
red:     \033[1;31m
yellow:  \033[0;33m
reset:   \033[0m
*/

void printBlue(char *string) {
	printf("\n\033[1;34m%s\033[0m", string);
}

void printYellow(char *string) {
    printf("\033[0;33m%s\033[0m", string);
}

void printRed(char *string) {
	fprintf(stderr, "\033[1;31m%s\033[0m", string);
}

void printGreen(char *string) {
	printf("\033[0;32m%s\033[0m", string);
}