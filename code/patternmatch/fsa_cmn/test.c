// wordlist from:
// http://www-personal.umich.edu/~jlawler/wordlist.html

#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hs_fsa.h"



//#define PTN_MAX 8000

#define ERROR_CHECK(ret)    do {\
    if((ret)) {\
        printf("\033[5;33mcheck error: %s #%d\033[0m\n", __FILE__, __LINE__);\
        return (ret);}\
    } while(0)

#define ERROR_CHECK_END(ret)    do {\
    if((ret)) {\
        printf("\033[5;33mcheck error: %s #%d\033[0m\n", __FILE__, __LINE__);\
        goto END;}\
    } while(0)


fsa_pattern_t*  g_ptns;
//char**    g_words = NULL;
uint32_t  g_ptns_cnt  = 0; 


char* trim(char *str);
int make_ptns(const char* path, uint32_t n_ptns);
int base_test();
int speed_test(const char* path, uint32_t n_ptns, uint32_t n_times);


int match_callback(uint32_t ptn_id, uint64_t offset, void* user_data)
{
#ifdef __GNUC__
    printf("%u %lu %p\n", ptn_id, offset, user_data);
#else
    printf("%u %llu %p\n", ptn_id, offset, user_data);
#endif
    return 0;
}


int make_ptns(const char* path, uint32_t n_ptns)
{
    FILE* fp;
    uint32_t i, len;
    char *ret, tmp[32];
    
    fp = fopen(path, "r");
    if(NULL == fp)
        return -1;

    g_ptns = (fsa_pattern_t*) malloc(sizeof(fsa_pattern_t) * n_ptns); 
    for(i=0; i<n_ptns; i++)
    {
        g_ptns[i].ptn = (uint8_t*) malloc(32);
        g_ptns[i].ptn_len = 0;
        g_ptns[i].ptn_id = 0;
    }

    g_ptns_cnt = 0;
    do
    {
        ret = fgets(tmp, 32, fp);
        if(ret == NULL)
            break;
        trim(tmp);
        len = strlen(tmp);
        if(len < 5)
            continue;

        memcpy((void*)g_ptns[g_ptns_cnt].ptn, tmp, len);
        g_ptns[g_ptns_cnt].ptn_len = len;
        g_ptns[g_ptns_cnt].ptn_id = g_ptns_cnt;
        g_ptns_cnt++;
    }
    while(ret != NULL && g_ptns_cnt < n_ptns);

    fclose(fp);
    return 0;
}

int main(int argc, char** argv)
{
    int ret;
    uint32_t n_ptns = 0, n_times = 0;
    char path[300] = {0};

    if(argc == 1)
        ret = base_test();
    else
    {
        if(argc > 1)
            n_ptns = strtoul(argv[1], NULL, 10);
        if(argc > 2)
            n_times = strtoul(argv[2], NULL, 10);
        if(argc > 3)
            strcpy(path, argv[3]);
            

        if(n_ptns < PTN_MAX)
            n_ptns = PTN_MAX;
        if(n_times < 10)
            n_times = 10;
        if(strlen(path) < 1)
            strcpy(path, "twilight.txt");
        ret = speed_test(path, n_ptns, n_times);
    }
    
    ERROR_CHECK(ret);

    return 0;
}

static int base_test_func(const char* data, fsa_format_e format,
                         fsa_pattern_t* ptns, uint32_t cnt)
{
    fsa_error_t ret;
    fsa_t fsa;
    uint64_t memsize;
    uint8_t* mem = NULL;
    uint32_t i;

    ret = fsa_init(&fsa, format, 0);
    ERROR_CHECK(ret);

    for(i=0; i<cnt; i++)
    {
        ret = fsa_add_pattern(&fsa, ptns[i].ptn, ptns[i].ptn_len, i);
        ERROR_CHECK(ret);
    }

    ret = fsa_need_memsize(&fsa, &memsize);
    ERROR_CHECK(ret);
    
    mem = (uint8_t*) malloc(memsize);
    ret = fsa_compile(&fsa, mem, memsize);
    ERROR_CHECK_END(ret);

    ret = fsa_search(&fsa, (const uint8_t*)data, strlen(data),
                    match_callback, (void*)0x12345);
    ERROR_CHECK_END(ret);

    fsa_deinit(&fsa);
    ERROR_CHECK_END(ret);

END:
    if(mem != NULL)
        free(mem);
    return 0;
}

int base_test()
{
    fsa_error_t ret;
    char txt[] = "ushers";
    fsa_pattern_t ptns[4] =
    {
        {(uint8_t*)"he", 2, 0},
        {(uint8_t*)"she", 3, 0},
        {(uint8_t*)"his", 3, 0},
        {(uint8_t*)"hers", 4, 0}
    };

    printf("NFA_LIST:\n");
    ret = base_test_func(txt, NFA_LIST, ptns, 4);
    ERROR_CHECK(ret);

    printf("DFA_LIST:\n");
    ret = base_test_func(txt, DFA_LIST, ptns, 4);
    ERROR_CHECK(ret);
    
    printf("DFA_FULL_MATRIX:\n");
    ret = base_test_func(txt, DFA_FULL_MATRIX, ptns, 4);
    ERROR_CHECK(ret);

    printf("DFA_BANDED_MATRIX:\n");
    ret = base_test_func(txt, DFA_BANDED_MATRIX, ptns, 4);
    ERROR_CHECK(ret);

    return 0;
}


int speed_test(const char* path, uint32_t n_ptns, uint32_t n_times)
{
    int ret;
    uint8_t* data;
    uint32_t datalen;
    FILE* fp;
    fsa_t fsa;
    uint64_t memsize;
    uint8_t* mem = NULL;
    struct timeval t1, t2;
    uint64_t t, tall;
    uint32_t i, loop;


    ret = make_ptns("wordlist.txt", n_ptns);
    ERROR_CHECK(ret); 

    fp = fopen(path, "r");
    if(fp == NULL)
        return -1;

    fseek(fp, 0L, SEEK_END);
    datalen = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    if(datalen < 1)
        goto END;
    
    data = (uint8_t*) malloc(datalen);
    if(fread(data, 1, datalen, fp) != datalen)
        goto END;

    ret = fsa_init(&fsa, DFA_FULL_MATRIX, 0);
    ERROR_CHECK_END(ret);

    for(i=0; i<g_ptns_cnt; i++)
    {
        ret = fsa_add_pattern(&fsa, g_ptns[i].ptn, g_ptns[i].ptn_len, i);
        ERROR_CHECK_END(ret);
    }

    memsize = 0ULL;
    ret = fsa_need_memsize(&fsa, &memsize);
    ERROR_CHECK_END(ret);
    
    mem = (uint8_t*) malloc(memsize);
    ret = fsa_compile(&fsa, mem, memsize);
    ERROR_CHECK_END(ret);

    //printf("ptn cnt: %u\n", fsa_get_pattern_count(&fsa));
    //printf("state cnt: %u\n", fsa_get_state_count(&fsa));
    //printf("memsize: %llu\n", memsize);


    tall = 0;
    for(loop=0; loop<n_times; loop++)
    {
        gettimeofday(&t1, NULL);
        for(i=0; i<2000000; i++)
            ret = fsa_search(&fsa, data, datalen, 
                    match_callback, (void*)0x123456);
        gettimeofday(&t2, NULL);
        t = 1000000 * ( t2.tv_sec - t1.tv_sec ) + t2.tv_usec - t1.tv_usec;
#ifdef __GNUC__
        printf("time used: %lu ms\n", t);
#else
        printf("time used: %llu ms\n", t);
#endif
        tall += t;
        //sleep(1);
    }
    printf("avg time: %f\n", (double)tall/n_times);
    ERROR_CHECK_END(ret);

    fsa_deinit(&fsa);
    ERROR_CHECK_END(ret);

END: 
    if(mem != NULL)
        free(mem);
    fclose(fp);
    return 0;
}


#define IS_SPACE(c)  ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
 
char* trim(char *str)
{
    size_t len;
    char *p, *end;

    p = str;
    if (p == NULL)
        return NULL;

    while(IS_SPACE(*p))
        p++;
    len = strlen(p);
    if (len < 1)
        return str;

    end = p + len - 1;
    while(IS_SPACE(*end))
        end--;
    *(++end) = '\0';

    end = p;
    str = p;
    while(*end != '\0')
        *(p++) = *(end++);
    *p = '\0';

    return str;
}



