#include "include.h"

struct{
    char Start;
    char Stop;
	char Print;
	char Time;
	char Measure;
	char NewFile;
	char Message;
	char SubData[SUBDATA_NUM];
	char MainData;
	int PartStr;
    int PartEnd;
    int PartChange;
    char Settime;
	char SDCommand;
	char Command;
    char Counting;
}Flag;

struct{
    int diff;
    int diff_before;
    int in;
    int out;
    char count;
}AdcReg;

struct{
    long int time_count;
    double mA_curr[AVG_CNT];
    double mV_curr[AVG_CNT];
    double mW_curr[AVG_CNT];
    double mA_curr_avg;
    double mV_curr_avg;
    double mW_curr_avg;
}MsrData, PreMsrData;

struct{
    int date;
    int hour;
    int min;
    int sec;
}CurrentTime, MobileTime, CompareTime;

char RealTime[20];
char *RT=RealTime;
char DateTime[20];
char *DT=DateTime;

char TempString[30];
char *TMPSTR=TempString;

int fd;
int sensor_temp;
int sensor_stab;
long int main_count;

char CommandA[100]={'\0',};
char *CMDA=CommandA;
unsigned char indexA = 0;
char CommandB[100]={'\0',};
char *CMDB=CommandB;
unsigned char indexB = 0;

char CommandB_Subname[100]={'\0',};

LogDataSet MainData;
LogDataSet SubData[SUBDATA_NUM];

timer_t record_timer;

FILE* fp;

static void timer_handler ( int sig, siginfo_t *si, void *uc);
static int makeTimer(timer_t *timerID, int sec, int msec);
int delTimer(timer_t *timerID);

void *thread_uart(void *data);

void InitTime (void);
void SetDiffTime (char* realtime);
//void subDataInit();
void subDataInit(int i);
void CalcCurrentTime (struct tm* time_temp);

/* Delay Compensate */
int getDiffusec (char *pre_time, char *cur_time);
void delayCompensator (char *pre_time, char *char_time);

/*  Test Function */
void observePass10sec (void);
void getdoubledt(char *dt);

int main ()
{
    pthread_t uart_thread;
    int thr_id;
    int status;
    int temp_data = 0;
    int i;
    int adc_sw = 0;

    char temp_key_input;
    char key;
    
    char pre_time_us[20] = {0};

    if(wiringPiSetup() == -1)
    {
        fprintf (stdout, "Unable to start wiringPi: %s\n", strerror(errno));
        return 1;
    }

    if(!bcm2835_init())
    {
        fprintf (stdout, "Unable to start bcm2835\n");
        return -1;
    }
    bcm2835_spi_init ();

    if ((fd = serialOpen ("/dev/ttyAMA0", 115200)) < 0)
    {
        fprintf (stdout, "Unable to open serial device: %s\n", strerror (errno));
        return 1 ;
    }  
    
    thr_id = pthread_create (&uart_thread,NULL,thread_uart,(void *)&temp_data);
    if(thr_id < 0)
    {
        perror("thread create error : ");
        exit(0);
    }
    
    InitTime();

	serialPuts(fd, "RESET\n");
    serialPutchar(fd, '#');
	
    while(1)
    {
        if(adc_sw == 0)
        {
            sensor_temp = read_mcp3208_adc(0, SINGLE_INPUT);
            AdcReg.diff=sensor_temp;
            adc_sw = 1;
        }
        else
        {
            sensor_temp = read_mcp3208_adc(2, DIFFERENCE_INPUT);
            AdcReg.out=sensor_temp;
            adc_sw = 0;
        }
        if(Flag.Settime!=0){
            SetDiffTime (CommandB);
            Flag.Settime = 0;
            Flag.Time = 1;
        }
        else if(Flag.NewFile==1)
		{
			NewFile(RealTime,DateTime);
			Flag.NewFile=0;
		}
/*
		if(Flag.SDCommand==1)
		{
			SDCommand(CMDB);
			Flag.SDCommand=0;
		}
*/
        else if(Flag.Start!=0){
            makeTimer(&record_timer, 0, TIME_INTERVAL_MS);
            Flag.Start=0;
            pre_time_us[0]=0;
		}
        else if(Flag.Message!=0)
		{
			Message(Flag.Message,CommandB,MainData);
			Flag.Message=0;
		}
        /*
		if(Flag.Time==1)
		{
			char RTC[9]={0};
            
            time_t current_time;
            struct tm *time_temp;

            time( &current_time);
            time_temp = localtime(&current_time);
			
            CalcCurrentTime(time_temp);

			sprintf(RTC,"%02d:%02d:%02d",CurrentTime.hour,CurrentTime.min,CurrentTime.sec);
            
            if(fp!=NULL)               
                fprintf(fp,"&IS,RealTime-%s, ,IS*\n",RTC);
            
            serialPuts(fd, RTC);
            serialPutchar(fd, 0x0A);
            serialPutchar(fd, 0x0D);
            serialPutchar(fd, '#');
			Flag.Time=0;
		}*/
        else if(Flag.PartEnd!=0)
		{
			if(Flag.SubData[(Flag.PartEnd-1)%SUBDATA_NUM]==0)
			    SubENDMSG(Flag.PartEnd-1, CommandB,SubData[(Flag.PartEnd-1)%SUBDATA_NUM]);
			Flag.PartEnd=0;
            subDataInit((Flag.PartEnd-1)%SUBDATA_NUM);
		}
        else if(Flag.PartStr!=0)
		{
			if(Flag.SubData[(Flag.PartStr-1)%SUBDATA_NUM]==1){
                SubSTRMSG(Flag.PartStr-1, CommandB);
                strcpy(CommandB_Subname, CommandB);
            }
			Flag.PartStr=0;
        }
        else if(Flag.PartChange!=0) 
        {
			if(Flag.SubData[(Flag.PartChange-1)%SUBDATA_NUM]==0&&Flag.SubData[(Flag.PartChange)%SUBDATA_NUM]==1){
			    SubENDMSG(Flag.PartChange-1, CommandB_Subname,SubData[(Flag.PartChange-1)%SUBDATA_NUM]);
                subDataInit((Flag.PartEnd-1)%SUBDATA_NUM);
                SubSTRMSG(Flag.PartChange, CommandB);
                strcpy(CommandB_Subname, CommandB);
            }
			Flag.PartChange=0;
        }
        else if(Flag.Time==1)
        {
            char RTC[20]={0};

            time_t current_time;
            struct tm *time_temp;

            time( &current_time);
            time_temp = localtime(&current_time);

            CalcCurrentTime(time_temp);

            sprintf(RTC,"%02d:%02d:%02d",CurrentTime.hour,CurrentTime.min,CurrentTime.sec);

            if(fp!=NULL)
                fprintf(fp,"&IS,RealTime-%s, ,IS*\n",RTC);
            serialPuts(fd, RTC);
            serialPutchar(fd, 0x0A);
            serialPutchar(fd, 0x0D);
            serialPutchar(fd, '#');
            Flag.Time=0;
        }
        else if(Flag.Stop!=0){
            delTimer(record_timer);
            fclose(fp);
            fp = NULL;
            Flag.Stop=0;
		}
//        observePass10sec (); // temp test
        else if(Flag.Print==1)
		{
            char time_us[20] = {0};

            /*  Compensation  */
            if(fp!=NULL){
                getdoubledt(time_us);
                delayCompensator(pre_time_us,time_us);
                fprintf(fp,"&MD,%8d,%8d,%8d,MD*,%s\n",(int)MsrData.mA_curr_avg,(int)MsrData.mV_curr_avg,(int)MsrData.mW_curr_avg,time_us);
            }
            Flag.Print=0;
        }
	}
    while (1);
}

void *thread_uart(void *data)
{
    char serial_data = 0;

    while(1){
        serial_data = serialGetchar (fd);
        if(serial_data!=0xff){
            fflush (stdout);
            
            if(serial_data=='$')
            {
                for(indexA=0;indexA<100;indexA++)CommandA[indexA]='\0';
                indexA=0;
                Flag.Command=1;
            }
            else if(Flag.Command==1)
            {
                if(serial_data=='@')
                {
                    CommandA[indexA]=serial_data;
                    Flag.Command=3;        
                }
                else if(serial_data=='#')
                {
                    CommandA[indexA]=serial_data;
                    for(indexB=0;indexB<100;indexB++)CommandB[indexB]='\0';
                    indexB=0;
                    Flag.Command=2;//sub command         
                }
                else
                {
                    CommandA[indexA]=serial_data;
                    indexA++;
                    if(indexA==100)
                    {
                        Flag.Command=0;
                    }
                }
            }
            else if(Flag.Command==2)
            {
                if(serial_data=='@')
                {
                    CommandA[indexA]=serial_data;
                    Flag.Command=4;       
                }       
                else
                {
                    CommandB[indexB]=serial_data;
                    indexB++;
                    if(indexB==100)
                    {
                        Flag.Command=0;
                    }
                }
            }
            else;
            if(Flag.Command==3)
            {
                if(strncmp(CMDA,"START",5)==0)
                {
                    //Measure_Start;
                    if(Flag.Counting == 0){
                       //makeTimer(&record_timer, 0, 20);
                        if(fp!=NULL){
                            Flag.Start=1;
                            Flag.Time=1;
                            Flag.Counting = 1;
                        } else{
                            serialPuts(fd, "No file\n");
                            serialPutchar(fd, '#');
                        }
                    }
                }
                else if(strncmp(CMDA,"STOP",4)==0)
                {
                    //Measure_Stop;
                    if(Flag.Counting == 1){
                        //delTimer(record_timer);
                        Flag.Stop=1;
                        Flag.Time=1;
                        Flag.Counting = 0;
                    }
                }
                else if(strncmp(CMDA,"NEWFILE",7)==0)Flag.NewFile=1;
                else if(strncmp(CMDA,"WRRT",4)==0)Flag.Time=1;
                else;
                Flag.Command=0;
            }
            else if(Flag.Command==4)
            {
                if(strncmp(CMDA,"SDMSG",5)==0)Flag.Message=1;
                else if(strncmp(CMDA,"MSGSTR",6)==0)
                {
                    Flag.Message=2;
                    Flag.MainData=1;
                }
                else if(strncmp(CMDA,"MSGEND",6)==0)
                {
                    Flag.Message=3;
                    Flag.MainData=0;
                    MainData.count = 0;
                }
                else if(strncmp(CMDA,"SUBSTR",6)==0)
                {
                    int temp;
                    temp=((CommandA[6]-'0')*1000 + (CommandA[7]-'0')*100 + (CommandA[8]-'0')*10 + CommandA[9]-'0');
                    Flag.SubData[temp%SUBDATA_NUM]=1;
                    Flag.PartStr=temp+1;
                }
                else if(strncmp(CMDA,"SUBEND",6)==0)
                {
                    int temp;
                    temp=((CommandA[6]-'0')*1000 + (CommandA[7]-'0')*100 + (CommandA[8]-'0')*10 + CommandA[9]-'0');
                    Flag.SubData[temp%SUBDATA_NUM]=0;
                    Flag.PartEnd=temp+1;
//                    subDataInit(); // if several subpart are ocurred simultanenously deleate this function
                }
                else if(strncmp(CMDA,"SUBCHANGE",9)==0)
                {
                    int temp;
                    temp=((CommandA[9]-'0')*1000 + (CommandA[10]-'0')*100 + (CommandA[11]-'0')*10 + CommandA[12]-'0');
                    Flag.SubData[temp%SUBDATA_NUM]=0;
//                    subDataInit(); // if several subpart are ocurred simultanenously deleate this function
                    Flag.SubData[(temp+1)%SUBDATA_NUM]=1;
                    Flag.PartChange=temp+1;
                }
                else if(strncmp(CMDA,"SETTIME",7)==0)Flag.Settime=1;
                else;
                Flag.Command=0;
            }
        }
    }
}

int makeTimer(timer_t *timerID, int sec, int msec)
{
    struct sigevent te;
    struct sigaction sa;
    struct itimerspec its;
    int sigNo = SIGRTMIN;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(sigNo, &sa, NULL) == -1)
    {
        printf("sigaction error\n");
        return -1;
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    te.sigev_value.sival_ptr = timerID;
    
    timer_create(CLOCK_REALTIME, &te, timerID);

    its.it_interval.tv_sec = sec;
    its.it_interval.tv_nsec = msec * 1000000;
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = msec * 1000000;

    timer_settime(*timerID, 0, &its, NULL);

    return 0;
}

int delTimer(timer_t *timerID)
{
    if (timer_delete (timerID) < 0){
        fprintf (stdout, "[%d]: %s\n", __LINE__, strerror (errno));
        exit (errno);
    }
    else{
        main_count = 0;
        return 0;
    }
}

void timer_handler ( int sig, siginfo_t *si, void *uc)
{
//    printf("main_count : %d\n",main_count++);
    calc_elec_inform();
}

double Current(int reg_diff)
{
    double mA_current;
    double unit_current=0.3052;
    if(abs(AdcReg.diff_before-reg_diff)>3) // is 3 proper ?? need to modify
    {
        mA_current=unit_current*(double)reg_diff;
        AdcReg.diff_before=reg_diff;
    }
    else
    {
        mA_current=unit_current*(double)AdcReg.diff_before;
    }
    return mA_current;
}

double Volt(int reg_out)
{
    double mV_volt;
    double unit_volt=1.2207; // 5*1000/4096

    mV_volt=unit_volt*(double)reg_out;

    return mV_volt;
}

double Power(double current, double volt)
{
    double uW_watt;
    
    uW_watt=current*volt;
    
    return uW_watt;
}

void calc_elec_inform (void)
{
    static int tmp_cnt=0;
	int i=0;
    double mW_curr_tmp = 0;
    double mA_curr_tmp = 0;
    double mV_curr_tmp = 0;

	tmp_cnt=MsrData.time_count%AVG_CNT;
	
	if(tmp_cnt==(AVG_CNT-1))Flag.Print=1;

	MsrData.mA_curr[tmp_cnt]=Current(AdcReg.diff);
	MsrData.mV_curr[tmp_cnt]=Volt(AdcReg.out);
	MsrData.mW_curr[tmp_cnt]=Power(MsrData.mA_curr[tmp_cnt], MsrData.mV_curr[tmp_cnt])/1000;
	MsrData.time_count++;
	if(Flag.MainData==1)
	{	
		MainData.mW_this=MsrData.mW_curr[tmp_cnt];
		MainData.mW_total=(MainData.mW_avg*MainData.count++)+MainData.mW_this;
		MainData.mW_avg=MainData.mW_total/MainData.count;
	}
	for(i=0;i<SUBDATA_NUM;i++)
	{
		if(Flag.SubData[i]==1)
		{	
			SubData[i].mW_this=MsrData.mW_curr[tmp_cnt];
			SubData[i].mW_total=(SubData[i].mW_avg*SubData[i].count++)+SubData[i].mW_this;
			SubData[i].mW_avg=SubData[i].mW_total/SubData[i].count;
		}
	}
	tmp_cnt=(MsrData.time_count-1)%AVG_CNT;
	if(tmp_cnt==(AVG_CNT-1)){
        Flag.Print=1;
		for(i=0;i<AVG_CNT;i++){
            mW_curr_tmp+=MsrData.mW_curr[i];
            mA_curr_tmp+=MsrData.mA_curr[i];
            mV_curr_tmp+=MsrData.mV_curr[i];
        }
		MsrData.mW_curr_avg = mW_curr_tmp/AVG_CNT;
		MsrData.mA_curr_avg = mA_curr_tmp/AVG_CNT;
		MsrData.mV_curr_avg = mV_curr_tmp/AVG_CNT;
    }
}

void GetTime (char *RT_temp, char *DT_temp)
{
    time_t current_time;
    struct tm *time_temp;
    
    char tempA[30];
    char tempB[30];
    
    time( &current_time);
    time_temp = localtime(&current_time);

    CalcCurrentTime(time_temp);
    
    sprintf(RT_temp,"%02d%02d%02d%02d",CurrentTime.date,CurrentTime.hour,CurrentTime.min,CurrentTime.sec);
    sprintf(DT_temp,"%d/%02d/%02d\n%02d:%02d:%02d\n",(time_temp->tm_year)+1900,(time_temp->tm_mon)+1,CurrentTime.date,CurrentTime.hour,CurrentTime.min,CurrentTime.sec);
    
    serialPuts(fd, DT_temp);
    serialPutchar(fd, '#');
}

void FileCreate (char *Name)
{
    char temp_name[50];
    int i;
    sprintf(temp_name,"/home/pi/data/%s.csv",Name);
    
    fp = fopen(temp_name,"w");
    if(fp != NULL){
        serialPuts(fd, "File created\n");
        serialPutchar(fd, '#');
    }
    else{
        serialPuts(fd, "Fail creating\n");
        serialPutchar(fd, '#');
    }
}
void WriteTime(char *DT_temp)
{
    if(fp!=NULL){
        fprintf(fp,"%s",DT_temp);
        serialPuts(fd, "Write Time OK\n");
        serialPutchar(fd, '#');
    } else{
        fprintf(stdout,"writeTime error!\n");
        serialPuts(fd, "Write Time Fail\n");
        serialPutchar(fd, '#');
        return;
    }
}
void NewFile(char *RT_temp, char *DT_temp)
{
    GetTime(RT_temp,DT_temp);
    FileCreate(RT_temp);
    WriteTime(DT_temp);
}
void Message(int flag, char *CMD_temp, LogDataSet DataSet)
{
    int i=0;
    int temp_flag=0;
    
    if(fp!=NULL){
        switch(flag){
            case 1	: fprintf(fp,"&IS,%s,	,IS*\n",CMD_temp); break;
            case 2	: fprintf(fp,"&IS,%s,STR,IS*\n",CMD_temp); break;
            case 3	: fprintf(fp,"&IS,%s,END,IS*\n",CMD_temp); break;
            default	: fprintf(fp,"&IS,%s,	,IS*\n",CMD_temp); break;
        }
        if(flag==3){
            fprintf(fp,"&IS,%8d,END,IS*\n",(int)DataSet.mW_avg);
            serialPrintf(fd,"mW_avg%5d mW\n#",(int)DataSet.mW_avg);
            serialPrintf(fd,"@MP%d\n#",(int)DataSet.mW_avg);
            //fprintf(fp,"%8d,END",(int)DataSet.mW_avg); // to bluetooth
        }
    }
    else{
        fprintf(stdout,"Message error!\n");
    }
    
}
void SubSTRMSG(int flag, char CMD_temp[])
{
    if(fp!=NULL){
        char RTC[20]={0};

        time_t current_time;
        struct tm *time_temp;

        time( &current_time);
        time_temp = localtime(&current_time);

        CalcCurrentTime(time_temp);

        sprintf(RTC,"%02d:%02d:%02d",CurrentTime.hour,CurrentTime.min,CurrentTime.sec);

        fprintf(fp,"&IS,%s,S%dSTR,",CMD_temp,flag);
        fprintf(fp,"RealTime-%s,IS*\n",RTC);
    }else{
        fprintf(stdout,"SubSTRMSG error!\n");
    }
}
void SubENDMSG(int flag, char CMD_temp[], LogDataSet DataSet)
{
    if(fp!=NULL){
        fprintf(fp,"&IS,%s,S%dEND,",CMD_temp,flag);
        fprintf(fp,"%8d,IS*\n",(int)DataSet.mW_avg,flag);
        serialPrintf(fd,"mW_avg%5d mW\n#",(int)DataSet.mW_avg);
        serialPrintf(fd,"@SP%04d%d\n#",flag,(int)DataSet.mW_avg);
        //fprintf(fp,"%8d,S%dEND",(int)DataSet.mW_avg,flag); // to bluetooth
    }
    else{
        fprintf(stdout,"SubENDMSG error!\n");
    }
}

void InitTime (void)
{
    time_t current_time;
    struct tm *time_temp;
    int temp_date, temp_hour, temp_min, temp_sec;

    time( &current_time);
    time_temp = localtime(&current_time);

    MobileTime.date = time_temp->tm_mday;
    MobileTime.hour = time_temp->tm_hour;
    MobileTime.min  = time_temp->tm_min;
    MobileTime.sec  = time_temp->tm_sec;

    CompareTime.date = time_temp->tm_mday;
    CompareTime.hour = time_temp->tm_hour;
    CompareTime.min  = time_temp->tm_min;
    CompareTime.sec  = time_temp->tm_sec;
}

void SetDiffTime (char* realtime)
{
    time_t current_time;
    struct tm *time_temp;
    int temp_date, temp_hour, temp_min, temp_sec; 
    
    time( &current_time);
    time_temp = localtime(&current_time);
    
    MobileTime.date = (realtime[0]-'0')*10 + realtime[1]-'0';
    MobileTime.hour = (realtime[2]-'0')*10 + realtime[3]-'0';
    MobileTime.min  = (realtime[4]-'0')*10 + realtime[5]-'0';
    MobileTime.sec  = (realtime[6]-'0')*10 + realtime[7]-'0';
    CompareTime.date = time_temp->tm_mday;
    CompareTime.hour = time_temp->tm_hour;
    CompareTime.min  = time_temp->tm_min;
    CompareTime.sec  = time_temp->tm_sec;
}

void CalcCurrentTime (struct tm* time_temp)
{
    CurrentTime.date = MobileTime.date + time_temp->tm_mday - CompareTime.date;
    CurrentTime.hour = MobileTime.hour + time_temp->tm_hour - CompareTime.hour;
    CurrentTime.min  = MobileTime.min + time_temp->tm_min - CompareTime.min;
    CurrentTime.sec  = MobileTime.sec + time_temp->tm_sec - CompareTime.sec;

    if (CurrentTime.sec < 0)
    {
        CurrentTime.min --;
        CurrentTime.sec += 60;
    }
    if (CurrentTime.min < 0)
    {
        CurrentTime.hour --;
        CurrentTime.min += 60;
    }
    if (CurrentTime.hour < 0)
    {
        CurrentTime.date --;
        CurrentTime.hour += 24;
    }

    if(CurrentTime.sec >= 60)
    {
        CurrentTime.min ++;
        CurrentTime.sec -= 60;
    }
    if(CurrentTime.min >=60)
    {
        CurrentTime.hour ++;
        CurrentTime.min -=60;
    }
    if(CurrentTime.hour >=24)
    {
        CurrentTime.date ++;
        CurrentTime.hour -=24;
    }
}

void bcm2835_spi_init (void)
{
    bcm2835_gpio_fsel(CS_MCP3208,BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set (CS_MCP3208);
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder (BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode (BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider (BCM2835_SPI_CLOCK_DIVIDER_65536);
}

int read_mcp3208_adc (int adcChannel, int mode)
{
    uint8_t buff[3] = { 0 };
    int adcValue = 0;

    buff[0] = (0x04 | ((adcChannel & 0x07) >> 7))+0x02*mode;
    buff[1] = ((adcChannel & 0x07) << 6);

    bcm2835_gpio_clr (CS_MCP3208);
    bcm2835_spi_transfern (buff, 3);
    buff[1] = 0x0f & buff[1];
    adcValue =  (buff[1] << 8) | buff[2];
    bcm2835_gpio_set (CS_MCP3208);

    return adcValue;
}
/*  Test Function  */
void observePass10sec (void)
{
    time_t current_time;
    struct tm *time_temp;
    int temp_date, temp_hour, temp_min, temp_sec;
    static int old_sec, current_sec;
    time( &current_time);
    time_temp = localtime(&current_time);

    old_sec = current_sec;
    current_sec = time_temp->tm_sec;
    
    if(old_sec != current_sec && !((CompareTime.sec)%10 - (current_sec)%10))
        Flag.Time=1;
}

void getdoubledt(char *dt)
{
    struct timeval val;
    struct tm *ptm;

    gettimeofday(&val, NULL);
    ptm = localtime(&val.tv_sec);
    
    CalcCurrentTime(ptm);
    
    memset(dt , 0x00 , sizeof(dt));

    sprintf(dt, "%02d:%02d:%02d.%06ld"
            , CurrentTime.hour,CurrentTime.min,CurrentTime.sec
            , val.tv_usec);
}

int getDiffusec (char *pre_time, char *cur_time)
{
    if(pre_time[0]==0 || cur_time[0]==0)
        return 0;
    int diff_sec = cur_time[7]-pre_time[7];
    int diff_usec=(cur_time[9]*100+cur_time[10]*10+cur_time[11])-(pre_time[9]*100+pre_time[10]*10+pre_time[11]);
	int unit = AVG_CNT * TIME_INTERVAL_MS;

    if(diff_sec<0) diff_sec += 10;
    if(diff_usec<0){ diff_usec += 1000; diff_sec--; }

    if((diff_usec%unit)>(unit/2)) return diff_sec*(1000/unit)+(diff_usec/unit+1) ;
    else return diff_sec*(1000/unit)+diff_usec/unit ; // count return if you change frequency change this function
}

/*
int getDiffusec (char *pre_time, char *cur_time)
{
    if(pre_time[0]==0 || cur_time[0]==0)
        return 0;
    int diff_sec = cur_time[7]-pre_time[7];
    int diff_usec=(cur_time[9]*10+cur_time[10])-(pre_time[9]*10+pre_time[10]);

    if(diff_sec<0) diff_sec += 10;
    if(diff_usec<0){ diff_usec += 100; diff_sec--; }

    if(diff_usec%10>5) return diff_sec*10+(diff_usec/10+1) ;
    else return diff_sec*10+diff_usec/10 ; // count return if you change frequency change this function
}
*/

void delayCompensator (char *pre_time, char *cur_time)
{
    int miss_count;
    int i,j;
    double mA_expect;
    double mV_expect;
    double mW_expect;
    
	if(pre_time[0]==0) {strcpy(pre_time,cur_time); return;}

    miss_count = getDiffusec(pre_time, cur_time)-1; 

    mA_expect = (MsrData.mA_curr_avg + PreMsrData.mA_curr_avg)/2;
    mV_expect = (MsrData.mV_curr_avg + PreMsrData.mV_curr_avg)/2;
    mW_expect = (MsrData.mW_curr_avg + PreMsrData.mW_curr_avg)/2;
    
    for(j=0;j<miss_count;j++){
        if(fp!=NULL)
            fprintf(fp,"&MD,%8d,%8d,%8d,MD*,%s,%s\n",(int)mA_expect,(int)mV_expect,(int)mW_expect,cur_time,"Delay Compensated");
    
        if(Flag.MainData==1)
        {
            MainData.mW_this=mW_expect;
            MainData.mW_total=(MainData.mW_avg*MainData.count++)+MainData.mW_this;
            MainData.mW_avg=MainData.mW_total/MainData.count;
        }
        for(i=0;i<SUBDATA_NUM;i++)
        {
            if(Flag.SubData[i]==1)
            {
                SubData[i].mW_this=mW_expect;
                SubData[i].mW_total=(SubData[i].mW_avg*SubData[i].count++)+SubData[i].mW_this;
                SubData[i].mW_avg=SubData[i].mW_total/SubData[i].count;
            }
        }
    }
    
    PreMsrData.mA_curr_avg = MsrData.mA_curr_avg;
    PreMsrData.mV_curr_avg = MsrData.mV_curr_avg;
    PreMsrData.mW_curr_avg = MsrData.mW_curr_avg;

    strcpy(pre_time,cur_time);
}

void subDataInit(int i)
{
        Flag.SubData[i]=0;
        SubData[i].mW_avg = 0;
        SubData[i].count = 0;
}
/*
void subDataInit()
{
    int i;
    for(i=0;i<SUBDATA_NUM;i++)
    {
        Flag.SubData[i]=0;
        SubData[i].mW_avg = 0;
        SubData[i].count = 0;
    }
}
*/
