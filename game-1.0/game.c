#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include "boards.h"
#include "images/grayBlock.h"
#include "images/redBlock.h"
#include "images/ball.h"
#include "images/platform.h"

#define RGB(r,g,b) ((r<<11) | (g<<5) | (b<<0))

struct Image
{
	int x;
	int y;
	int height;
	int width;
	uint16_t* imageData;
};


int driver_descriptor;
int mem;
int platform_sx=0;

void gamepad_handler(int signum) {
	printf("Entered handlerrr\n");
	unsigned btn_id;
	int readstatus = read(driver_descriptor, &btn_id, sizeof(int));

	if(readstatus < 0) {
		printf("Failed to read from device file\n");
	}
	else {
		//press left button
		if(btn_id == 1) {
			printf("AAA\n");
			platform_sx = -1;}
		//right button
		else if(btn_id == 4)
			platform_sx = 1;

		printf("Read %u from device file\n", btn_id);
	}
}

void initDriver()
{
	driver_descriptor = open("/dev/driver_gamepad", O_RDONLY);
	if(driver_descriptor < 0) {
		printf("Failed to open device file\n");
	}
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_handler = gamepad_handler;
	action.sa_flags = 0;

	int sigstatus = sigaction(SIGIO, &action, NULL);
	fcntl(driver_descriptor, F_SETOWN, getpid());
	fcntl(driver_descriptor, F_SETFL, fcntl(driver_descriptor, F_GETFL) | FASYNC);
	if(sigstatus == SIG_ERR) {
		printf("Failed to initialize signal handler\n");
	}
}

float absolute(float x)
{
     if (x < 0)
         x = -x;
     return x;
}

float sqrtf(float x)
{
	float guess = 3.333;

    while(absolute(guess*guess - x) >= 0.0001 )
        guess = ((x/guess) + guess) / 2.0;

    return guess;
}

void drawImage(uint16_t *screen,int x,int y,int dx,int dy,const uint16_t *image)
{
	int i,u;
	int yCount=0,xCount=0;
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
		{
			screen[u+(i*320)] = image[xCount+(yCount*dx)];
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
		{
			screen[u+(i*320)] = color;
		}
	}
}

void clearRect(uint16_t *screen,int x,int y,int dx,int dy)
{
	int i,u;
	for(i=y;i<y+dy;i++)
	{
		for(u=x;u<x+dx;u++)
		{
			screen[u+(i*320)] = 0;
		}
	}
}

int* loadMap(uint16_t *screen,int level)
{
	int i=0,u=0;
	int* squareData = (int*)malloc(16*16*sizeof(int));

	int * board;

	switch(level)
	{
	case 0:
		board = board1;
		break;

	default:
		board = board1;
		break;
	}

	for(i=0;i<16;i++)
	{
		for(u=0;u<16;u++)
		{
			squareData[u+(i*16)] = board[u+(i*16)];
			switch(board[u+(i*16)])
			{
				case 1:
					drawImage(screen,u*grayBlockSizeX,i*grayBlockSizeY,grayBlockSizeX,grayBlockSizeY,grayBlockData);
					break;
				case 2:
					drawImage(screen,u*redBlockSizeX,i*redBlockSizeY,redBlockSizeX,redBlockSizeY,redBlockData);
					break;
			}
		}
	}
	return squareData;
}

void refreshScreen(int fd)
{
	struct fb_copyarea rect;

	rect.dx = 0;
	rect.dy = 0;
	rect.width = 320;
	rect.height = 240;
	ioctl(fd,0x4680,&rect);
}

int countRemovableBlocks(int *level)
{
	int i,u;
	int count = 0;
	for(i=0;i<16;i++)
	{
		for(u=0;u<16;u++)
		{
			if(level[u+(i*16)] >= 2)
				count++;
		}
	}
	return count;
}

int min(int a,int b)
{
	if(a<b)
		return a;
	return b;
}

void playLevel(uint16_t *screen,int *level,int fd)
{
	int removeCount = countRemovableBlocks(level);
	float ballx=160,bally=180;
	float ballsx=0.0053277*3.0,ballsy=0.056*2.0;

	float platform_x = 150;
	float platform_y = 220;
	float platform_speed = 0.02;

	struct fb_copyarea rect;

	drawImage(screen,(int)platform_x,(int)platform_y,platformSizeX,platformSizeY,platformData);

	rect.dx = (int)platform_x;
	rect.dy = (int)platform_y;
	rect.width = platformSizeX;
	rect.height = platformSizeY;
	ioctl(fd,0x4680,&rect);

	while(removeCount>0)
	{
		int drawBallx,drawBally;

		int oldBallx = (int)ballx;
		int oldBally = (int)bally;

		clearRect(screen,oldBallx,oldBally,ballSizeX,ballSizeY);

		ballx+=ballsx;
		bally+=ballsy;

		drawBallx = (int)ballx;
		drawBally = (int)bally;

		if(platform_sx < -0.0001 && platform_sx > 0.0001)
		{
			int oldPlatform = (int)platform_x;
			platform_x += platform_sx*platform_speed;
			int drawPlatform = (int)platform_x;

			clearRect(screen,oldPlatform,(int)platform_y,platformSizeX,platformSizeY);

			drawImage(screen,(int)platform_x,(int)platform_y,platformSizeX,platformSizeY,platformData);

			rect.dx = oldPlatform;
			rect.dy = (int)platform_y;
			rect.width = platformSizeX;
			rect.height = platformSizeY;
			ioctl(fd,0x4680,&rect);


		}

		if(drawBallx + ballSizeX > 320)
		{
			drawBallx = 320 - ballSizeX;
			ballsx = -ballsx;
		}
		else if(drawBallx < 0)
		{
			drawBallx = 0;
			ballsx = -ballsx;
		}

		if(drawBally + ballSizeY > 240)
		{
			drawBally = 240 - ballSizeY;
			ballsy = -ballsy;
		}
		else if(drawBally < 0)
		{
			drawBally = 0;
			ballsy = -ballsy;
		}

		float ballCenterX = ballx + (ballSizeX/2);
		float ballCenterY = bally + (ballSizeY/2);

		if(bally < 120)
		{
			int leftIndex,rightIndex,downIndex,upIndex;

			leftIndex = ((int)ballx)/grayBlockSizeX;
			rightIndex = ((int)ballx + ballSizeX)/grayBlockSizeX;
			upIndex = ((int)bally)/grayBlockSizeY;
			downIndex = ((int)bally + ballSizeY)/grayBlockSizeY;
			int centerXIndex = ((int)ballCenterX)/grayBlockSizeX;
			int centerYIndex = ((int)ballCenterY)/grayBlockSizeY;

			int collisionIndex = -1;
			int flatCollision = 0;
			int xDir = 0;
			int yDir = 0;

			if(ballsx < 0)
				xDir = -1;
			else
				xDir = 1;
			if(ballsy < 0)
				yDir = -1;
			else
				yDir = 1;

			if(level[centerXIndex + xDir + (centerYIndex * 16)] != 0)
				flatCollision = 1;
			else if(level[centerXIndex + ((centerYIndex + yDir) * 16)] != 0)
				flatCollision = 1;

			if(flatCollision)
			{
				if(level[rightIndex + (centerYIndex * 16)] != 0)
				{
					ballsx = -ballsx;
					collisionIndex = rightIndex + (centerYIndex * 16);
				}
				else if(level[leftIndex + (centerYIndex * 16)] != 0)
				{
					ballsx = -ballsx;
					collisionIndex = leftIndex + (centerYIndex * 16);
				}
				if(level[centerXIndex + (upIndex * 16)] != 0)
				{
					ballsy = -ballsy;
					collisionIndex = centerXIndex + (upIndex * 16);
				}
				else if(level[centerXIndex + (downIndex * 16)] != 0)
				{
					ballsy = -ballsy;
					collisionIndex = centerXIndex + (downIndex * 16);
				}
			}
			else
			{
				int cornerX = -1;
				int cornerY = -1;

				//lower right
				if(level[centerXIndex + 1 + ((centerYIndex + 1)*16)] != 0)
				{
					int tempCornerX = (centerXIndex + 1) * grayBlockSizeX;
					int tempCornerY = (centerYIndex + 1) * grayBlockSizeY;
					if(tempCornerX > ballx && tempCornerX < ballx + ballSizeX && tempCornerY > bally && tempCornerY < bally + ballSizeY)
					{
						cornerX = tempCornerY;
						cornerY = tempCornerY;
						collisionIndex = centerXIndex + 1 + ((centerYIndex + 1)*16);
					}
				}
				//lower left
				if(level[centerXIndex - 1 + ((centerYIndex + 1)*16)] != 0)
				{
					int tempCornerX = (centerXIndex) * grayBlockSizeX;
					int tempCornerY = (centerYIndex + 1) * grayBlockSizeY;
					if(tempCornerX > ballx && tempCornerX < ballx + ballSizeX && tempCornerY > bally && tempCornerY < bally + ballSizeY)
					{
						cornerX = tempCornerY;
						cornerY = tempCornerY;
						collisionIndex = centerXIndex - 1 + ((centerYIndex + 1)*16);
					}
				}
				//upper right
				if(level[centerXIndex + 1 + ((centerYIndex - 1)*16)] != 0)
				{
					int tempCornerX = (centerXIndex + 1) * grayBlockSizeX;
					int tempCornerY = (centerYIndex) * grayBlockSizeY;
					if(tempCornerX > ballx && tempCornerX < ballx + ballSizeX && tempCornerY > bally && tempCornerY < bally + ballSizeY)
					{
						cornerX = tempCornerY;
						cornerY = tempCornerY;
						collisionIndex = centerXIndex + 1 + ((centerYIndex - 1)*16);
					}
				}
				//upper left
				if(level[centerXIndex - 1 + ((centerYIndex - 1)*16)] != 0)
				{
					int tempCornerX = (centerXIndex) * grayBlockSizeX;
					int tempCornerY = (centerYIndex) * grayBlockSizeY;
					if(tempCornerX > ballx && tempCornerX < ballx + ballSizeX && tempCornerY > bally && tempCornerY < bally + ballSizeY)
					{
						cornerX = tempCornerY;
						cornerY = tempCornerY;
						collisionIndex = centerXIndex - 1 + ((centerYIndex - 1)*16);
					}
				}

				if(cornerX > 0 && cornerY > 0)
				{
					float centerDistanceSquared = ((cornerX - ballCenterX)*(cornerX - ballCenterX)) + ((cornerY - ballCenterY)*(cornerY - ballCenterY));
					float tempValue = ((ballsx * (ballCenterX - cornerX)) + (ballsy * (ballCenterY - cornerY))) / centerDistanceSquared;
					ballsx = ballsx - (2 * tempValue * (ballCenterX - cornerX));
					ballsy = ballsy - (2 * tempValue * (ballCenterY - cornerY));

					//ballx = oldBallx;
					//bally = oldBally;

					//level[collisionIndex] = 0;
				}
			}

			if(collisionIndex >= 0 && level[collisionIndex] > 0)
			{
				level[collisionIndex] = 0;
				removeCount--;
				clearRect(screen,(collisionIndex % 16) * grayBlockSizeX,(collisionIndex / 16) * grayBlockSizeY,grayBlockSizeX,grayBlockSizeY);
				rect.dx = (collisionIndex % 16) * grayBlockSizeX;
				rect.dy = (collisionIndex / 16) * grayBlockSizeY;
				rect.width = grayBlockSizeX;
				rect.height = grayBlockSizeY;
				ioctl(fd,0x4680,&rect);

				if(removeCount <= 0)
					return;
			}
		}
		else if(bally + ballSizeY >= platform_y)
		{
			float platformCenter = platform_x + platformSizeX / 2;
			float platformDist = ballx - platformCenter;
			float outSpeedx = (platformDist / platformSizeX)*0.056*2.0;
			float outSpeedy = -sqrtf(1 - (outSpeedx * outSpeedx))*0.056*2.0;

			bally = platform_y - ballSizeY;

			ballsx = outSpeedx;
			ballsy = outSpeedy;
		}

		//First quadrant
		/*if(ballsy < 0 && ballsx < 0)
		{
			//Left of right side of block
			if(leftIndex != centerXIndex)
			{
				//Underneath block which is a corner
				if(upIndex != centerYIndex)
				{
					if(level[leftIndex + (upIndex*16)] != 0 && level[centerXIndex + (upIndex*16)] != 0)
					{
						ballsy = -ballsy;
					}
				}
			}
		}
		else if(ballsy < 0 && ballsx >= 0)
		{

		}*/

		drawImage(screen,drawBallx,drawBally,ballSizeX,ballSizeY,ballData);

		rect.dx = min(drawBallx,oldBallx);
		rect.dy = min(drawBally,oldBally);
		rect.width = ballSizeX + ((int)ballsx + 1);
		rect.height = ballSizeY + ((int)ballsy + 1);
		ioctl(fd,0x4680,&rect);
	}
}

int main(int argc, char *argv[])
{
	int fd = open("/dev/fb0",O_RDWR);
	int level = 0;
	int * currentLevel;
	if(!fd)
		printf("File was not opened");
	else
	{
		uint16_t *screen = mmap(0,320*240*2,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		clearRect(screen,0,0,320,240);

		initDriver();

		currentLevel = loadMap(screen,level);

		refreshScreen(fd);

		playLevel(screen,currentLevel,fd);

		printf("Hello World, I'm game!\n");
	}
	exit(EXIT_SUCCESS);
}
