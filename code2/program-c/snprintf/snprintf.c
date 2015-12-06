#include "string.h"
#include "stdio.h"
#include "stdlib.h"
main()
{
    char *buf;
    int usedlen;
    int totallen;
    int leftlen = 0;
    int i;
    usedlen = 0;
    totallen = 64;
    buf = malloc(64);
    memset(buf,0,64);
    for(i = 0;i < 15;i++)
    {
    
    leftlen = 64 - usedlen;
    usedlen += snprintf(buf + usedlen,leftlen,"123456789");
    
    printf("i=%2d usedlen=%4d leftlen=%20d buf=%s pbuf+72=%s\n",i,usedlen,leftlen,buf,buf+72);
    }
    free(buf);
}
