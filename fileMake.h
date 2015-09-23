#ifndef __FILEMAKE_H__
#define __FILEMAKE_H__

typedef struct{
	int count;
	double mW_this;
	double mW_total;
	double mW_avg;
}LogDataSet;

void GetTime (char *RT_temp, char *DT_temp);
void FileCreate (char *Name);
void WriteTime(char *DT_temp);
void NewFile(char *RT_temp, char *DT_temp);
void Message(int flag, char *CMD_temp, LogDataSet DataSet);

void SubENDMSG(int flag, char CMD_temp[], LogDataSet DataSet);
void SubSTRMSG(int flag, char CMD_temp[]);

#endif
