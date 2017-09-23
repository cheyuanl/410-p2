#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>

int main(){
    int len;
    int status = 4;
    char *buf;
    int error_code;
    buf = "Hello world!";
    len = strlen(buf);
    MAGIC_BREAK;
    error_code = print(len,buf);
    lprintf("Expect to see \"%s\" in console." 
            "The return code of print is %d \n", buf, error_code);
    
    lprintf("You can expect to see the process exit with status: %d\n", status);
    return status;
}
