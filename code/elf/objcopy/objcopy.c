#include <stdio.h>
extern char _binary_testfile_start[];
extern char _binary_testfile_size[];

#define UNZIP_FILE(name,dstFile) \
do\
{\
extern char _binary_## name ##_start[];\
extern char _binary_## name ##_size[];\
    printf("filesize:%lu\n",(unsigned long)_binary_## name ##_size);\
    FILE *fp;\
    if ((fp = fopen(dstFile, "w")) == NULL)\
        printf("cannot open file!\n");\
    fwrite(_binary_## name  ##_start,1,(size_t)_binary_## name ##_size,fp);\
    fclose(fp);\
\
}while(0);

main()
{
    UNZIP_FILE(testfile,"a2.tar.gz");
    getchar();
    remove("a2.tar.gz");
}
