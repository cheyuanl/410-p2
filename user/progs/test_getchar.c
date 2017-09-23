#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>

int main(){
    char c;
    printf("Enter character: ");
    c = getchar();
    printf("The input char is %c \n",c);
    return 0;
}
