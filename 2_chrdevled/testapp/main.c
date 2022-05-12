#include <stdio.h>
#include "asm-generic/fcntl.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "stdlib.h"
#include "string.h"

#define  LED_FILE "/dev/chrdevled"

int main(int argc, char *argv[])
{
    if( argc < 3 )
     {
         printf("Please input(0:open  1:close): \n \"./testapp w 1\"\n\"./testapp w 0\"\n ") ;
         return -1 ;
     }
     int fd = open(LED_FILE, O_WRONLY) ;
     if( fd < 0)
     {
         printf("open dev fail.\n") ;
         return -2 ;
     }
     int len = write(fd, argv[2], 1) ;
     printf("write data = %s, len = %d\n",argv[2],  len) ;
    return 1 ;
}