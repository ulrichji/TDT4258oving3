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
#include <time.h>

#include "boards.h"
#include "images/grayBlock.h"
#include "images/redBlock.h"
#include "images/ball.h"
#include "images/platform.h"

#include "screenutil.h"

#define RGB(r,g,b) ((r<<11) | (g<<5) | (b<<0))

#define STARTBALLX 160
#define STARTBALLY 180
#define BALLSTARTSPEED 120	//in pixels per second
#define PLATFORMSTARTX (SCREENWIDTH / 2) - 20 	//Platform size should be constant
#define PLATFORMSTARTY 220
#define PLATFORMSTARTSPEED 150 //in pixels per second
#define FPS 60
#define SLEEPTIME 1000000 / FPS 

#define SW1 0x1
#define SW3 0x4

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
	//printf("Entered handlerrr\n");
	unsigned btn_id;
	int readstatus = read(driver_descriptor, &btn_id, sizeof(int));

	if(readstatus < 0) {
		printf("Failed to read from device file\n");
	}
	else {
		//press left button
		if(btn_id & SW1)
			platform_sx = -1;
		//right button
		else if(btn_id & SW3)
			platform_sx = 1;
		else
			platform_sx = 0;
		//printf("Read %u from device file\n", btn_id);
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

int* loadMap(uint16_t *screen,int level)
{
	int i=0,u=0;
	int* squareData = (int*)malloc(BOARDSQUARESWIDE*BOARDSQUARESHIGH*sizeof(int));

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

	for(i=0;i<BOARDSQUARESHIGH;i++)
	{
		for(u=0;u<BOARDSQUARESWIDE;u++)
		{
			squareData[u+(i*BOARDSQUARESWIDE)] = board[u+(i*BOARDSQUARESWIDE)];

			uint16_t mask = NOMASK;

			switch(board[u+(i*BOARDSQUARESWIDE)])
			{
				case 2:
					mask = REDMASK;
					break;
				case 3:
					mask = GREENMASK;
					break;
				case 4:
					mask = BLUEMASK;
					break;
			}
			if(board[u+(i*BOARDSQUARESWIDE)] != 0)
				drawImage(screen,u*grayBlockSizeX,i*grayBlockSizeY,grayBlockSizeX,grayBlockSizeY,grayBlockData, mask);
		}
	}
	return squareData;
}

void refreshScreen(int fd)
{
	struct fb_copyarea rect;

	rect.dx = 0;
	rect.dy = 0;
	rect.width = SCREENWIDTH;
	rect.height = SCREENHEIGHT;
	ioctl(fd,0x4680,&rect);
}

int countRemovableBlocks(int *level)
{
	int i,u;
	int count = 0;
	for(i=0;i<BOARDSQUARESHIGH;i++)
	{
		for(u=0;u<BOARDSQUARESWIDE;u++)
		{
			//since 0 is no block and 1 is an unremovable block
			if(level[u+(i*BOARDSQUARESWIDE)] >= 2)
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

int max(int a,int b)
{
	if(a>b)
		return a;
	return b;
}

long getTime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (1000000 * tv.tv_sec) + tv.tv_usec;
}

struct ImageDescriptor
{
	int width;
	int height;
};

struct MovableGameObject
{
	float x;
	float y;
	float sx;
	float sy;
	int width;
	int height;
	struct ImageDescriptor image;
};

int applyCollision(struct MovableGameObject* ball,int *level)
{
	int collisionIndex = -1;
	float ballCenterX = ball->x + (ballSizeX/2);
	float ballCenterY = ball->y + (ballSizeY/2);

	if(ball->x + ballSizeX + 1 > SCREENWIDTH)
	{
		ball->x = SCREENWIDTH - ballSizeX - 1;
		ball->sx = -ball->sx;
	}
	else if(ball->x < 0)
	{
		ball->x = 0;
		ball->sx = -ball->sx;
	}

	if(ball->y + ballSizeY + 1 > SCREENHEIGHT)
	{
		ball->y = SCREENHEIGHT - ballSizeY - 1;
		ball->sy = -ball->sy;
	}
	else if(ball->y < 0)
	{
		ball->y = 0;
		ball->sy = -ball->sy;
	}

	if(ball->y < 120)
	{
		int leftIndex,rightIndex,downIndex,upIndex;

		leftIndex = ((int)ball->x)/grayBlockSizeX;
		rightIndex = ((int)ball->x + ballSizeX)/grayBlockSizeX;
		upIndex = ((int)ball->y)/grayBlockSizeY;
		downIndex = ((int)ball->y + ballSizeY)/grayBlockSizeY;
		int centerXIndex = ((int)ballCenterX)/grayBlockSizeX;
		int centerYIndex = ((int)ballCenterY)/grayBlockSizeY;

		int flatCollision = 0;
		int xDir = 0;
		int yDir = 0;

		if(ball->sx < 0)
			xDir = -1;
		else
			xDir = 1;
		if(ball->sy < 0)
			yDir = -1;
		else
			yDir = 1;

		//a flat collision is a collision not involving the corners of the square.
		//Here we check if the index in the right direction is a block
		if(level[centerXIndex + xDir + (centerYIndex * 16)] != 0)
			flatCollision = 1;
		else if(level[centerXIndex + ((centerYIndex + yDir) * 16)] != 0)
			flatCollision = 1;

		if(flatCollision)
		{
			//Check if it is a flat collision to the right
			if(level[rightIndex + (centerYIndex * 16)] != 0)
			{
				ball->sx = -ball->sx;
				ball->x = (rightIndex * grayBlockSizeX) - ballSizeX;
				collisionIndex = rightIndex + (centerYIndex * 16);
			}
			else if(level[leftIndex + (centerYIndex * 16)] != 0)
			{
				ball->sx = -ball->sx;
				ball->x = grayBlockSizeX + (leftIndex * grayBlockSizeX);
				collisionIndex = leftIndex + (centerYIndex * 16);
			}
			if(level[centerXIndex + (upIndex * 16)] != 0)
			{
				ball->sy = -ball->sy;
				ball->y = grayBlockSizeY + (upIndex * grayBlockSizeY);
				collisionIndex = centerXIndex + (upIndex * 16);
			}
			else if(level[centerXIndex + (downIndex * 16)] != 0)
			{
				ball->sy = -ball->sy;
				ball->y = (downIndex * grayBlockSizeY) - ballSizeY;
				collisionIndex = centerXIndex + (downIndex * 16);
			}
		}
		else
		{
			int cornerX = -1;
			int cornerY = -1;

			//lower right
			if(level[centerXIndex + 1 + ((centerYIndex + 1) * BOARDSQUARESWIDE)] != 0)
			{
				int tempCornerX = (centerXIndex + 1) * grayBlockSizeX;
				int tempCornerY = (centerYIndex + 1) * grayBlockSizeY;
				if(tempCornerX > ball->x && tempCornerX < ball->x + ballSizeX && tempCornerY > ball->y && tempCornerY < ball->y + ballSizeY)
				{
					cornerX = tempCornerX;
					cornerY = tempCornerY;
					collisionIndex = centerXIndex + 1 + ((centerYIndex + 1) * BOARDSQUARESWIDE);
				}
			}
			//lower left
			else if(level[centerXIndex - 1 + ((centerYIndex + 1) * BOARDSQUARESWIDE)] != 0)
			{
				int tempCornerX = (centerXIndex) * grayBlockSizeX;
				int tempCornerY = (centerYIndex + 1) * grayBlockSizeY;
				if(tempCornerX > ball->x && tempCornerX < ball->x + ballSizeX && tempCornerY > ball->y && tempCornerY < ball->y + ballSizeY)
				{
					cornerX = tempCornerX;
					cornerY = tempCornerY;
					collisionIndex = centerXIndex - 1 + ((centerYIndex + 1) * BOARDSQUARESWIDE);
				}
			}
			//upper right
			else if(level[centerXIndex + 1 + ((centerYIndex - 1) * BOARDSQUARESHIGH)] != 0)
			{
				int tempCornerX = (centerXIndex + 1) * grayBlockSizeX;
				int tempCornerY = (centerYIndex) * grayBlockSizeY;
				if(tempCornerX > ball->x && tempCornerX < ball->x + ballSizeX && tempCornerY > ball->y && tempCornerY < ball->y + ballSizeY)
				{
					cornerX = tempCornerX;
					cornerY = tempCornerY;
					collisionIndex = centerXIndex + 1 + ((centerYIndex - 1) * BOARDSQUARESWIDE);
				}
			}
			//upper left
			else if(level[centerXIndex - 1 + ((centerYIndex - 1) * BOARDSQUARESHIGH)] != 0)
			{
				int tempCornerX = (centerXIndex) * grayBlockSizeX;
				int tempCornerY = (centerYIndex) * grayBlockSizeY;
				if(tempCornerX > ball->x && tempCornerX < ball->x + ballSizeX && tempCornerY > ball->y && tempCornerY < ball->y + ballSizeY)
				{
					cornerX = tempCornerX;
					cornerY = tempCornerY;
					collisionIndex = centerXIndex - 1 + ((centerYIndex - 1) * BOARDSQUARESWIDE);
				}
			}

			if(cornerX > 0 && cornerY > 0)
			{
				float x = ballCenterX - cornerX;
				float y = ballCenterY - cornerY;
				float c = -2 * (ball->sx * x + ball->sy * y) / (x * x + y * y);
				ball->sx = ball->sx + (c * x);
				ball->sy = ball->sy + (c * y);
			}
		}
	}
	return collisionIndex;
}

void refreshRect(int x,int y,int width,int height,int fd)
{
	struct fb_copyarea rect;

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

	ioctl(fd,0x4680,&rect);
}

int playLevel(uint16_t *screen,int *level,int fd)
{
	int removeCount = countRemovableBlocks(level);
	float ballSpeed = BALLSTARTSPEED;		//The speed the ball is travelling in
	float platformSpeed = PLATFORMSTARTSPEED;//the horizontal speed of the platform
	float deltaTime = 1;
	struct MovableGameObject ball;			//The ball
	struct MovableGameObject platform;		//The platform

	ball.x = STARTBALLX;
	ball.y = STARTBALLY;
	ball.sx = ballSpeed * 0.1; //This is hardcoded and not correct. Will give different speed
	ball.sy = ballSpeed * 0.9; //This is hardcoded and not correct. Will give different speed

	platform.x = PLATFORMSTARTX;
	platform.y = PLATFORMSTARTY;
	platform.sx = 0;
	platform.sy = 0;	

	drawImage(screen,(int)platform.x,(int)platform.y,platformSizeX,platformSizeY,platformData, NOMASK);

	refreshRect((int)platform.x,(int)platform.y,platformSizeX,platformSizeY,fd);

	long lastTime = 0;
	long startTime = getTime();

	while(removeCount>0)
	{
		lastTime = startTime;
		startTime = getTime();

		deltaTime = (float)(startTime - lastTime) / 1000000.0;

		float oldPlatformx = platform.x;
		//float oldPlatformy = platform.y;

		platform.sx = platform_sx * platformSpeed * deltaTime;
		platform.x += platform.sx;

		if(platform.x < 0)
			platform.x = 0;
		else if(platform.x + platformSizeX > SCREENWIDTH)
			platform.x = SCREENWIDTH - platformSizeX;

		float oldBallx = ball.x;
		float oldBally = ball.y;

		ball.x += ball.sx * deltaTime;
		ball.y += ball.sy * deltaTime;

		int collisionIndex = applyCollision(&ball,level);

		//if we are colliding with platform
		if(ball.y + ballSizeY >= platform.y && 		//we are below platform
			ball.x + ballSizeX > platform.x && 			//Left side of ball is on platform
			ball.x < platform.x + platformSizeX)		//right side of ball is on platform
		{
			ball.y = platform.y - ballSizeY;
			ball.sx = ((ball.x + (ballSizeX / 2)) - (platform.x + (platformSizeX / 2))) / ((float)platformSizeX);
			ball.sy = -sqrtf(1 - (ball.sx * ball.sx));
			ball.sx *= ballSpeed;
			ball.sy *= ballSpeed;
		}

		//now repaint what we need to repaint
		if(collisionIndex >= 0 && level[collisionIndex] > 1)
		{
			level[collisionIndex] = 0;
			removeCount--;

			//there are no more blocks to remove, and the level is done
			if(removeCount <= 0)
				return 0;

			int blockPosX = (collisionIndex % BOARDSQUARESWIDE) * grayBlockSizeX;
			int blockPosY = (collisionIndex / BOARDSQUARESWIDE) * grayBlockSizeY;

			//clear the block);

			clearRect(screen,blockPosX,blockPosY,grayBlockSizeX,grayBlockSizeY);

			//Refresh screen at this position
			refreshRect(blockPosX,blockPosY,grayBlockSizeX,grayBlockSizeY,fd);
		}
		
		int drawBallx = (int)ball.x;
		int drawBally = (int)ball.y;

		//Clear the ball
		clearRect(screen,(int)oldBallx,(int)oldBally,ballSizeX,ballSizeY);
		//draw the ball
		drawImage(screen,drawBallx,drawBally,ballSizeX,ballSizeY,ballData, NOMASK);

		//refresh the screen at the ball position
		refreshRect(min(drawBallx,(int)oldBallx),		//the rect should include both the new and the old ball that is removed
			min(drawBally,(int)oldBally),
			ballSizeX + (int)absolute(drawBallx - oldBallx) + 1,
			ballSizeY + (int)absolute(drawBally - oldBally) + 1, 
			fd);
		/*printf("%d %d %d %d\n",min(drawBallx,(int)oldBallx)-1,		//the rect should include both the new and the old ball that is removed
			min(drawBally,oldBally)-1,
			ballSizeX + max(drawBallx - (int)oldBallx,(int)oldBallx - drawBallx),
			ballSizeY + max(drawBally - (int)oldBally,(int)oldBally - drawBally));*/

		if(platform.sx < -0.0001 || platform.sx > 0.0001)
		{
			if(platform.sx > 0)
				clearRect(screen,
					(int)oldPlatformx,	//Start at the old position
					(int)platform.y,
					(int)absolute((int)platform.x - (int)oldPlatformx),	//Clear the ammount it have moved
					platformSizeY);

			else
				clearRect(screen,
					(int)platform.x + platformSizeX,  //Start at the right side of the platform 
					(int)platform.y,
					(int)absolute((int)platform.x - (int)oldPlatformx), //Clear the ammount it have moved
					platformSizeY);

			//Draw the platform
			drawImage(screen,(int)platform.x,(int)platform.y,platformSizeX,platformSizeY,platformData, NOMASK);

			//refresh screen at position of platform
			refreshRect(min((int)platform.x, (int)oldPlatformx),	//The minimum of the new and old pos
				(int)platform.y,
				platformSizeX + (int)absolute(platform.x - oldPlatformx) + 1,	//Refresh the size of platform plus the ammount it has moved
				platformSizeY,
				fd);
			//refreshRect(0,(int)platform.y,SCREENWIDTH,platformSizeY,fd);

			/*printf("draw: %d %d %d %d \nrefresh: %d %d %d %d\n",(int)platform.x,(int)platform.y,platformSizeX,platformSizeY,
				min((int)platform.x, (int)oldPlatformx),
				(int)platform.y,
				platformSizeX + (int)absolute(platform.x - oldPlatformx) + 1,
				platformSizeY);*/
		}

		//refreshScreen(fd);

		long endTime = getTime();

		long sleepTime = SLEEPTIME - (endTime - startTime);
		/*long nextTime = endTime + sleepTime;
		if(sleepTime > 0)
			while(getTime() < nextTime);*/
		usleep((int)sleepTime);
	}
	return 0;
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
