#include <stdio.h>
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char *argv[])
{
    char *filename = argv[1] ; ;
    char writebuf[100], readbuf[100] ;
    int fd = -1 ;
    if( argc < 3)
    {
        printf("Please input: devname-w/r") ;
        return -1 ;
    }

    fd = open(filename, O_RDWR ) ;
    if(fd < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    if( !strcmp( argv[2], "w"))   //write数据
    {
        strcat(writebuf, argv[3]) ;
        ssize_t writelen = write(fd, writebuf, 50) ;
        if( writelen < 0 )
        {
            printf("Write date failed\n") ;
        }
        else
        {
            printf("Write data success.\n") ;
        }
    }
    else    //read
    {
        ssize_t readlen = read(fd, readbuf, 50) ;
        if( readlen < 0 )
        {
            printf("read date failed\n") ;
        }
        else
        {
            printf("read data success. readbuf= %s .\n", readbuf) ;
        }      
    }
    
    int retvalue = close(fd) ;
    if( retvalue < 0)
    {
        printf("Can't close file %s", filename) ;
        return -1 ;
    }
    else
    {
        printf("Close file %s success", filename) ;
    }
    return 0 ;
}