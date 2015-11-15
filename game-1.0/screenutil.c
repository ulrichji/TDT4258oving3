#include <stdint.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/types.h>

#include "screenutil.h"

/* Draw an image on the screen at the specified area and given and given mask.
 * Screen is the screen to draw to
 * x and y is the position of the image
 * dx and dy is the size in x and y direction of the image
 * image is an array of colors that shall be copied to screen
 * colorMask is an integer representing a mask of wich color channels that shall be displayed */
void drawImage(uint16_t *screen,int x,int y,int dx,int dy,const uint16_t *image, uint16_t colorMask)
{
	//used to count pixel position on screen
	int i,u;

	//used to count pixel position on image
	int yCount=0,xCount=0;
	//copy each pixel of the image to the screen on the given positions
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
		{
			//copy the pixel in the image to the pixel on the screen.
			screen[u+(i*SCREENWIDTH)] = image[xCount+(yCount*dx)] & colorMask;
			xCount++;
		}
		xCount = 0;
		yCount ++;
	}
}

/* Draw a rectangle on the screen at the specified area and given color.
 * Screen is the screen to draw to
 * x and y is the position of the rectangle
 * dx and dy is the size in x and y direction of the rectangle to draw
 * color is the 16 bit integer representation of the color */
void drawRect(uint16_t *screen,int x,int y,int dx,int dy,uint16_t color)
{
	int i,u;
	//loop through all the pixels in the rect and set each to the specified color
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
			screen[u+(i*SCREENWIDTH)] = color;
	}
}

/* Clear the screen at the specified area.
 * Screen is the screen that should be cleared from
 * x and y is the position of the rectangle
 * dx and dy is the size in x and y direction of the rectangle to clear*/
void clearRect(uint16_t *screen,int x,int y,int dx,int dy)
{
	int i,u;
	//Loop through all the pixels that should be cleared
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
			screen[u+(i*SCREENWIDTH)] = 0;
	}
}

//Refresh the screen at position x,y and dimensions width,height.
//fd is the frame buffer that shall be refreshed
void refreshRect(int x,int y,int width,int height,int fd)
{
	//the area that should be refreshed
	struct fb_copyarea rect;

	//set the area to the given parameters
	rect.dx = x;
	rect.dy = y + 1;
	rect.width = width;
	rect.height = height;

	//We need to check that we are within bounds of screen
	if(x < 0)
	{
		rect.dx = 0;	//set position to 0
		rect.width = rect.width + x;	//decrease width by ammount of negative x
		//if the width are negative, the whole rect is outside board and no refresh is needed
		if(rect.width <= 0)
			return;
	}
	//Outside the bounds on other side of screen
	else if(x + width > SCREENWIDTH)
	{
		rect.width = SCREENWIDTH - x;
		//if the width is negative, then we are outside the bounds of the screen
		if(rect.width <= 0)
			return;
	}

	if(y < 0)
	{
		rect.dy = 0;	//Set position to 0
		rect.height = rect.height + y;	//subtract the height by the ammount of rect that is outside of the bounds
		if(rect.height <= 0)
			return;
	}
	else if(y + height > SCREENHEIGHT)
	{
		//set the height to ammount of square that is outside of the screen bounds
		rect.height = SCREENHEIGHT - y;
		if(rect.height <= 0)
			return;
	}

	//refresh the screen with the given area
	ioctl(fd,0x4680,&rect);
}

//Refreshing the whole screen.
void refreshScreen(int fd)
{
	//the area to refresh
	struct fb_copyarea rect;

	//set starting position to 0, 0
	rect.dx = 0;
	rect.dy = 0;
	//Set the dimensions to the size of the screen.
	rect.width = SCREENWIDTH;
	rect.height = SCREENHEIGHT;
	//Refresh the screen.
	ioctl(fd,0x4680,&rect);
}
