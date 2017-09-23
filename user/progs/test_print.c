#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>

int main(){
    int len;
    char *buf="Hello world!";
    int error_code;
    //buf = "Hello world!";
    len = strlen(buf);
    error_code = print(len,buf);
    MAGIC_BREAK;
    lprintf("Expect to see \"%s\" in console." 
            "The return code of readline is %d \n", buf, error_code);
    
    return 0;
}
