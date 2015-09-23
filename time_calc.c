#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void getdoubledt (char *dt);
int getDiffusec (char *pre_time, char *cur_time);

char pre_time[20];
char cur_time[20];

int main (void)
{
    int i,j;
    getdoubledt(pre_time);
    getdoubledt(cur_time);

    while(1)
    {
        getdoubledt(cur_time);
        if(getDiffusec(pre_time, cur_time)>1){
            printf("diff time usec : %d\n%s - %s\n",getDiffusec(pre_time, cur_time),cur_time,pre_time);
            getdoubledt(pre_time);
        }
        for(i=0;i<100;i++)
            for(j=0;j<100;j++);
    }
    return 0;
}


void getdoubledt(char *dt)
{
    struct timeval val;
    struct tm *ptm;

    gettimeofday(&val, NULL);
    ptm = localtime(&val.tv_sec);

    memset(dt , 0x00 , sizeof(dt));

    sprintf(dt, "%02d:%02d:%02d.%06ld"
            , ptm->tm_hour, ptm->tm_min, ptm->tm_sec
            , val.tv_usec);
}

int getDiffusec (char *pre_time, char *cur_time)
{
    int diff_sec = cur_time[7]-pre_time[7];
    int diff_usec=(cur_time[9]*10+cur_time[10])-(pre_time[9]*10+pre_time[10]);
    
    if(diff_sec<0) diff_sec += 10;
    if(diff_usec<0){ diff_usec += 100; diff_sec--; }

    if(diff_usec%10>5) return diff_sec*10+(diff_usec/10+1) ;
    else return diff_sec*10+diff_usec/10 ;
}
