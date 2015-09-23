#ifndef __PPAM_H__
#define __PPAM_H__

#define AVG_CNT 2
#define TIME_INTERVAL_MS 5 
#define SUBDATA_NUM 10

double Current (int reg_diff);
double Volt (int reg_out);
double Power(double current, double volt);
void calc_elec_inform (void);

#endif
