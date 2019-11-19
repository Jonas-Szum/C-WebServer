#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    if (argc < 2) {
        printf("Please provide two string arguments\n");
        exit(1);
    }
    printf("Hello %s, would you like to play a game of %s?\n", argv[0], argv[1]);
}
