#include <stdio.h>
#include <stdlib.h>

char* scanString() {

    char str[256];
    int len;

    scanf(" %[^\n]%n",str,&len);

    char *good_string = malloc(len + 1);

    for (int i = 0; i < len + 1; i++){
        good_string[i] = str[i];
        if (str[i] == '\00')
            break;
    }
    return good_string;
};