/*
	calc.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "error.h"
#include "calc.h"
#include "os.h"


void calc::init()
{
	for(int i=0;i<10;i++)
	{
		warp_speed_table[i]=i*i*i*100;
	}
	srand(os::getseed());
}

long calc::random_int(long max)
{
	if(max==0)
		return 0;
	return (rand()%max);
}

void calc::getspeed(long spd,char* put)
{
	int j; //To extract the value from the loop

	j=0;
	for(int i=9;i>=0;i--)
	{
		if(spd>=warp_speed_table[i])
		{
			j=i;
			break;
		}
	}
	if(j==0)
		if(spd<10)
			sprintf(put,"0.0%ld c",spd);
		else
			sprintf(put,"0.%ld c",spd);
	else
		sprintf(put,"Warp %hd",j);
}

bool calc::data_arrays_equal(unsigned char* d1,unsigned char* d2,int n)
{
	for(int i=0;i<n;i++)
		if(d1[i]!=d2[i])
			return false;
	return true;		
}

void calc::encrypt_password(char* str)
{
	for(int i=0,j=strlen(str);i<j;i++)
		str[i]+=i+1;
}

long calc::warp_speed_table[10];
char calc::speed_string_buffer[33];
