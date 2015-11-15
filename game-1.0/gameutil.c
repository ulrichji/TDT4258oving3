#include <sys/time.h>
#include <time.h>

#include "gameutil.h"

//Returns the absolute value of x
float absolute(float x)
{
	//if x is negative return -x
     if (x < 0)
         x = -x;
     return x;
}

//Returns the square root of x.
//it is optimized for square root of 10
//Using newtons method to determine square root.
//starts with guess of 3.333 which is an approximation of sqrt(10)
float sqrtf(float x)
{
	float guess = 3.333;

	//check that the guess is within wanted presition
    while(absolute(guess*guess - x) >= 0.0001 )
    	//if not, compute the next iteration
        guess = ((x/guess) + guess) / 2.0;

    return guess;
}

//Returns the smallest of a and b
int min(int a,int b)
{
	if(a<b)
		return a;
	return b;
}

//Returns the largest of a and b
int max(int a,int b)
{
	if(a>b)
		return a;
	return b;
}

//Returns the microsecond of this day.
long getTime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (1000000 * tv.tv_sec) + tv.tv_usec;
}