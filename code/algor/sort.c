void maopao(int src[],int cnt)
{
    int i,j = 0;
    int tmp;
    for(i = 0 ;i < cnt; i++)
    {
        for(j = 0;j<cnt -i - 1;j++ )
        {
            if(src[j] > src[j+1])
            {
                tmp = src[j];
                src[j] = src[j+1];
                src[j+1] = tmp;
            }
        }
    }

}
void zhijie(int src[],int cnt)
{
    int i,j = 0;  
    int tmp  = 0;
    printf("zhijie\n");
    for(i = 1 ; i < cnt; i++) 
    {
        if(src[i] < src[i-1])
        {
            tmp = src[i];
            j = i;
            while(tmp < src[j-1] )
            {
                src[j]=src[j-1];
                j--;
            }
            src[j]=tmp;
        }
    }
}
main()
{
    int i  = 0;
    int src[10] = {5,7,1,3,0,100,89,44,3,4};
    //    maopao(src,10);
    zhijie(src,10);  
    for(i = 0;i<10;i++)
    {
        printf("%d ",src[i]);
    }
    printf("\n");
}
