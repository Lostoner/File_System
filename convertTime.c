#include<stdio.h>
#include<time.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>

int const Year_a[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int const Year_b[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int const Year_C[4] = { 366,365,365,365 };

typedef struct              //时间包
{
	int year;          //0--年
	int month;          //1--月
	int day;            //2--日
	int hour;           //3--时
	int Minute;         //4--分
	int Second;         //5--分
}TimePackge;

char* Tlocaltime(unsigned int Second, char* c)
{
	TimePackge Times;
	int i;

	Second += 28800;  //北京时间是8:00:00  60*60*8=28800秒
	Times.Second = Second % 60;
	Times.Minute = Second % 3600 / 60;
	Times.hour = Second / 3600 % 24;


	Times.month = 0;
	Times.day = 0;
	Second = Second / 86400;  //共有多少天,1天=86400秒
	Second += 2;              //每年是从第1天算起，所以加1天
	//printf("\n从:%d年1月1日到今有%d天", Times.year,Second);
	Times.year = Second / 1461;//4年
	Times.year = Times.year * 4 + 1970;//
	Second = Second % 1461;
	//printf("\n还有多少:%d天",Second);
	if (Second > 0) {
		i = 0;
		while (Second > Year_C[i]) {
			Second -= Year_C[i];
			Times.year++;
			//printf("\n日期:%d年%d天", Times.year,Second);
			i++;
		}
		i = 0;
		if (Times.year % 4 == 0) {  //润年
			while (Second > Year_b[i]) {
				Second -= Year_b[i];
				Times.month++;
				i = Times.month % 12;
			}
		}
		else {
			while (Second > Year_a[i]) {
				Second -= Year_a[i];
				Times.month++;
				i = Times.month % 12;
			}
		}
		Times.day = Second;
		Times.month++;
	}
	else {
		Times.year--;
		Times.day = 1;
		Times.month = 1;
	}
	//printf("%d-%d-%d %d:%d:%d\n", Times.year, Times.month, Times.day, Times.hour, Times.Minute, Times.Second);
	char c1[20],c2[20],c3[20],c4[20],c5[20],c6[20];
	itoa(Times.year,c1,10);
	itoa(Times.month,c2,10);
	itoa(Times.day - 1,c3,10);
	itoa(Times.hour,c4,10);
	itoa(Times.Minute,c5,10);
	itoa(Times.Second,c6,10);
	strcat(c1,"-");
	strcat(c1,c2);
	strcat(c1,"-");
	strcat(c1,c3);
	strcat(c1," ");
	strcat(c1,c4);
	strcat(c1,":");
	strcat(c1,c5);
	strcat(c1,":");
	strcat(c1,c6);
	strcpy(c,c1);
	// printf("%s\n", c1);
	//printf("%d",strlen(c1));
	return c;
}



