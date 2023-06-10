#include<stdio.h>
#include<string.h>
#include<sys/shm.h>
#include<time.h>
#include<stdlib.h>

//转换函数，将int类型转换成char *类型
void itoa(int i,char*string)
{
   int power,j;
   j=i;
   for(power=1;j>=10;j/=10)
     power*=10;
   for(;power>0;power/=10)
   {
     *string++='0'+i/power;
     i%=power;
   }
   *string='\0';
}

//get system time...
void get_cur_time(char * time_str)
{
   time_t timep;
   struct tm *p_curtime;
   char *time_tmp;
   time_tmp=(char *)malloc(20);
   memset(time_tmp,0,2);
   memset(time_str,0,20);
   time(&timep);
   p_curtime = localtime(&timep);
   strcpy(time_str,"(");
   itoa(p_curtime->tm_hour,time_tmp);
   strcat(time_str,time_tmp);
   strcat(time_str,":");
   itoa(p_curtime->tm_min,time_tmp);
   strcat(time_str,time_tmp);
   strcat(time_str,":");
   itoa(p_curtime->tm_sec,time_tmp);
   strcat(time_str,time_tmp);
   strcat(time_str,") ");
   free(time_tmp);
}
