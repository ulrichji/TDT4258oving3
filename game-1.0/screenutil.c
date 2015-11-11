#include <stdint.h>

#include "screenutil.h"

void drawImage(uint16_t *screen,int x,int y,int dx,int dy,const uint16_t *image)
{
	int i,u;
	int yCount=0,xCount=0;
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
		{
			screen[u+(i*SCREENWIDTH)] = image[xCount+(yCount*dx)];
			xCount++;
		}
		xCount = 0;
		yCount ++;
	}
}

void drawRect(uint16_t *screen,int x,int y,int dx,int dy,uint16_t color)
{
	int i,u;
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
			screen[u+(i*SCREENWIDTH)] = color;
	}
}

void clearRect(uint16_t *screen,int x,int y,int dx,int dy)
{
	int i,u;
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
			screen[u+(i*SCREENWIDTH)] = 0;
	}
}