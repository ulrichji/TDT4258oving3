#include <stdio.h>		//for standard io like printf
#include <stdlib.h>		//For malloc
#include <stdint.h>		//Standard data types
#include <unistd.h>		//For sleeping

#include <fcntl.h>		//Used for the fcntl in driver initialization
#include <string.h> 	//For memset
#include <signal.h>		//For signals in driver initialization
#include <errno.h>		//used for errortypes in driver initialization

#include <sys/types.h>	//driver file operations
#include <sys/mman.h>	//for memory mapping

#include "boards.h"
#include "images/grayBlock.h"
#include "images/ball.h"
#include "images/platform.h"

#include "screenutil.h"
#include "gameutil.h"

#define RGB(r,g,b) ((r<<11) | (g<<5) | (b<<0))

#define STARTBALLX 160
#define STARTBALLY 180
#define BALLSTARTSPEED 120	//in pixels per second
#define PLATFORMSTARTX (SCREENWIDTH / 2) - 20 	//Platform size should be constant
#define PLATFORMSTARTY 220
#define PLATFORMSTARTSPEED 150 //in pixels per second
#define FPS 60
#define SLEEPTIME 1000000 / FPS 

#define NUMBEROFLIVES 4

#define SW1 0x1
#define SW3 0x4

//the descriptor of the driver
int driver_descriptor;
int mem;
//the movement direction of the platform
int platform_sx=0;

void gamepad_handler(int signum) {
	//the buttons that are pressed
	unsigned btn_id;
	//Read the buttons from the gamepad driver.
	int readstatus = read(driver_descriptor, &btn_id, sizeof(int));

	if(readstatus < 0) {
		printf("Failed to read from device file\n");
	}
	else {
		//Left button pressed
		if(btn_id & SW1)
			platform_sx = -1;
		//Right button pressed
		else if(btn_id & SW3)
			platform_sx = 1;
		//The wanted buttons are not pressed
		else
			platform_sx = 0;
	}
}

void initDriver()
{
	//open the driver gamepad
	driver_descriptor = open("/dev/driver_gamepad", O_RDONLY);
	if(driver_descriptor < 0) {
		printf("Failed to open device file\n");
	}
	//Used to register the handler to the driver.
	struct sigaction action;

	//Set all bits in action to 0
	memset(&action, 0, sizeof(action));
	
	//The handler function to register
	action.sa_handler = gamepad_handler;
	//No flags
	action.sa_flags = 0;

	//Register handler. Store result in sigstatus
	int sigstatus = sigaction(SIGIO, &action, NULL);
	//Call the games process when something happens in driver
	fcntl(driver_descriptor, F_SETOWN, getpid());
	//Enable asynchronous notifications.
	fcntl(driver_descriptor, F_SETFL, fcntl(driver_descriptor, F_GETFL) | FASYNC);

	if(sigstatus == SIG_ERR) {
		printf("Failed to initialize signal handler\n");
	}
}

//Function will return a pointer to the copy of the level count.
int *loadMap(uint16_t *screen,int level)
{
	int i=0,u=0;
	//this will be the copy of the board
	int* squareData = (int*)malloc(BOARDSQUARESWIDE*BOARDSQUARESHIGH*sizeof(int));

	//this is a pointer to the board we are copying from
	int * board;

	//select the board corresponding to the level
	switch(level)
	{
	case 0:
		board = board1;
		break;
	case 1:
		board = board2;
		break;
	case 2:
		board = board3;
		break;
	case 3:
		board = board4;
		break;
	default:
		board = board1;
		break;
	}

	//copy board and draw the corresponding image
	for(i=0;i<BOARDSQUARESHIGH;i++)
	{
		for(u=0;u<BOARDSQUARESWIDE;u++)
		{
			//copy the data
			squareData[u+(i*BOARDSQUARESWIDE)] = board[u+(i*BOARDSQUARESWIDE)];

			//the mask of the block
			uint16_t mask = NOMASK;

			//select the mask of the color corresponding to the value of block
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
				case 5:
					mask = YELLOWMASK;
					break;
			}
			//Draw the image of the block with the specified mask if it is a solid block
			if(board[u+(i*BOARDSQUARESWIDE)] != 0)
				drawImage(screen,u*grayBlockSizeX,i*grayBlockSizeY,grayBlockSizeX,grayBlockSizeY,grayBlockData, mask);
		}
	}
	//return the level's data
	return squareData;
}

//count the blocks in the level that are removable
int countRemovableBlocks(int *level)
{
	int i;
	//the amount of removable blocks
	int count = 0;
	//go through all squares
	for(i=0;i<BOARDSQUARESHIGH*BOARDSQUARESWIDE;i++)
	{
		//since 0 is no block and 1 is an unremovable block
		if(level[i] >= 2)
			count++;
	}
	return count;
}

int applyCollision(struct MovableGameObject* ball,int *level)
{
	//set the default collision index
	int collisionIndex = -1;
	//calculate where the center of the ball is
	float ballCenterX = ball->x + (ballSizeX/2);
	float ballCenterY = ball->y + (ballSizeY/2);

	//if we are outside the board on the right side
	if(ball->x + ballSizeX + 1 > SCREENWIDTH)
	{
		//Snap ball to edge of sceen
		ball->x = SCREENWIDTH - ballSizeX - 1;
		//additive invert speed in x direction
		ball->sx = -ball->sx;
	}
	//if the ball is outside the board on the left side
	else if(ball->x < 0)
	{
		//snap ball to edge of screen
		ball->x = 0;
		//additive invert speed in x direction
		ball->sx = -ball->sx;
	}

	//If the ball is outside the bottom of the screen (not really needed since this really is a loss)
	if(ball->y + ballSizeY + 1 > SCREENHEIGHT)
	{
		//Snap ball to edge of screen
		ball->y = SCREENHEIGHT - ballSizeY - 1;
		//Additive invert speed in y direction
		ball->sy = -ball->sy;
	}
	//if the ball is outside the top of the screen
	else if(ball->y < 0)
	{
		//snap ball to the edge of the screen
		ball->y = 0;
		//additive invert the ball's y speed
		ball->sy = -ball->sy;
	}

	//If the ball is in the section of rectangles, we must check for collisions
	if(ball->y <= 16 * BOARDSQUARESHIGH)
	{
		//the block indices of the edges of the ball
		int leftIndex,rightIndex,downIndex,upIndex;

		leftIndex = ((int)ball->x)/grayBlockSizeX;
		rightIndex = ((int)ball->x + ballSizeX)/grayBlockSizeX;
		upIndex = ((int)ball->y)/grayBlockSizeY;
		downIndex = ((int)ball->y + ballSizeY)/grayBlockSizeY;
		//The block index where the center of the ball is
		int centerXIndex = ((int)ballCenterX)/grayBlockSizeX;
		int centerYIndex = ((int)ballCenterY)/grayBlockSizeY;

		//If the collision is a flat collision
		//A flat collision is a collision with a linear surface
		int flatCollision = 0;
		//The direction the ball is travelling in
		int xDir = 0;
		int yDir = 0;

		//Set the directions
		if(ball->sx < 0)
			xDir = -1;
		else
			xDir = 1;
		if(ball->sy < 0)
			yDir = -1;
		else
			yDir = 1;

		//a flat collision is a collision not involving the corners of the square.
		//Here we check if the index in the movement direction is a block
		if(level[centerXIndex + xDir + (centerYIndex * 16)] != 0)
			flatCollision = 1;
		else if(level[centerXIndex + ((centerYIndex + yDir) * 16)] != 0)
			flatCollision = 1;

		//If it is a flat collision, it cannot be a corner collision
		if(flatCollision)
		{
			//Check if it is a flat collision to the right
			if(level[rightIndex + (centerYIndex * 16)] != 0)
			{
				//additive invert speed in x direction
				ball->sx = -ball->sx;
				//snap ball to edge of block
				ball->x = (rightIndex * grayBlockSizeX) - ballSizeX;
				//Set the collision index
				collisionIndex = rightIndex + (centerYIndex * 16);
			}
			//Check if it is a flat collision to the left
			else if(level[leftIndex + (centerYIndex * 16)] != 0)
			{
				ball->sx = -ball->sx;
				ball->x = grayBlockSizeX + (leftIndex * grayBlockSizeX);
				collisionIndex = leftIndex + (centerYIndex * 16);
			}
			//Check if it is a flat collision on the top
			if(level[centerXIndex + (upIndex * 16)] != 0)
			{
				//Additive invert speed in y direction
				ball->sy = -ball->sy;
				//Snap ball to edge of block
				ball->y = grayBlockSizeY + (upIndex * grayBlockSizeY);
				collisionIndex = centerXIndex + (upIndex * 16);
			}
			//Check if it is a flat collision at the bottom
			else if(level[centerXIndex + (downIndex * 16)] != 0)
			{
				ball->sy = -ball->sy;
				ball->y = (downIndex * grayBlockSizeY) - ballSizeY;
				collisionIndex = centerXIndex + (downIndex * 16);
			}
		}
		//If not a flat collision, it is a corner collision.
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

					//warp ball
					ball->x = cornerX - ballSizeX;
					ball->y = cornerY - ballSizeY;
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

					//warp ball
					ball->x = cornerX;
					ball->y = cornerY - ballSizeY;
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

					//warp ball
					ball->x = cornerX - ballSizeX;
					ball->y = cornerY;
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

					//warp ball
					ball->x = cornerX;
					ball->y = cornerY;
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

//returns the color mask of the given lives.
uint16_t getMaskFromLives(int lives)
{
	switch(lives)
	{
	case 1:
		return REDMASK;
	case 2:
		return YELLOWMASK;
	case 3:
		return GREENMASK;
	default:
		return NOMASK;
	}
}

/* Function will play the specified level and return the number of lives left.
 * Screen is the screen to draw to
 * Level is the integer array representation of the board that will be played
 * Lives is the amount of lives left
 * fd is the frame buffer that should be drawn to.
*/
int playLevel(uint16_t *screen,int *level,int lives,int fd)
{
	//the amount of removable blocks for this level.
	int removeCount = countRemovableBlocks(level);
	float ballSpeed = BALLSTARTSPEED;		//The speed the ball is travelling in
	float platformSpeed = PLATFORMSTARTSPEED;//the horizontal speed of the platform
	
	//delta time is here the time the previous frame took in seconds.
	//multiply this with all movement.
	float deltaTime = 1;

	//the color mask for the ball. Default is no mask
	uint16_t ballMask = NOMASK;

	struct MovableGameObject ball;			//The ball
	struct MovableGameObject platform;		//The platform

	ball.x = STARTBALLX;
	ball.y = STARTBALLY;
	ball.sx = ballSpeed * 0.1;
	ball.sy = sqrtf((ballSpeed*ballSpeed) - (ball.sx * ball.sx)); //The required speed in y direction to have the wanted ball speed

	platform.x = PLATFORMSTARTX;
	platform.y = PLATFORMSTARTY;
	platform.sx = 0;
	platform.sy = 0;	

	//get the mask of the ball based on the lives.
	ballMask = getMaskFromLives(lives);

	//draw the platform
	drawImage(screen,(int)platform.x,(int)platform.y,platformSizeX,platformSizeY,platformData, NOMASK);
	//refresh screen at platform position
	refreshRect((int)platform.x,(int)platform.y,platformSizeX,platformSizeY,fd);

	//the time the last frame finished
	long lastTime = 0;
	//the start time of this frame
	long startTime = getTime();

	while(removeCount>0)
	{
		//the time the previous frame started
		lastTime = startTime;
		//This frame started at this time
		startTime = getTime();
		//delta time is the time in seconds the last frame lasted. Divide by 1000000 to get it from microseconds to seconds
		deltaTime = (float)(startTime - lastTime) / 1000000.0;

		//the previous position of platform
		float oldPlatformx = platform.x;
		
		//Set the speed of the platform based on what key is pressed
		platform.sx = platform_sx * platformSpeed;
		//Move the platform
		platform.x += platform.sx * deltaTime;

		//Set the platform to be within the screen if it has moved outside it.
		if(platform.x < 0)
			platform.x = 0;
		else if(platform.x + platformSizeX > SCREENWIDTH)
			platform.x = SCREENWIDTH - platformSizeX;

		//the previous position of the ball
		float oldBallx = ball.x;
		float oldBally = ball.y;

		//move the ball
		ball.x += ball.sx * deltaTime;
		ball.y += ball.sy * deltaTime;

		//Find and apply collision to ball
		int collisionIndex = applyCollision(&ball,level);

		//if we are colliding with platform
		if(ball.y + ballSizeY >= platform.y && 		//we are below platform
			ball.x + ballSizeX > platform.x && 			//Left side of ball is on platform
			ball.x < platform.x + platformSizeX)		//right side of ball is on platform
		{
			//warp the ball to the top of the platform
			ball.y = platform.y - ballSizeY;
			//set the x-speed of the platform according to where it hit the platform
			ball.sx = ((ball.x + (ballSizeX / 2)) - (platform.x + (platformSizeX / 2))) / ((float)platformSizeX);
			//Set speed in y-direction such that ball velocity is contained
			ball.sy = -sqrtf(1 - (ball.sx * ball.sx));
			//Since the current speeds are normalized, apply the speed of the ball
			ball.sx *= ballSpeed;
			ball.sy *= ballSpeed;
		}
		//The ball missed the platform
		else if(ball.y > platform.y)
		{
			//Decrement lives
			lives --;

			printf("Lost a life. Now you have %d lives\n",lives);

			//If we are out of lives. We have lost the game.
			if(lives <= 0)
				return 0;

			//get a new color for the ball based on the amount of lives
			ballMask = getMaskFromLives(lives);

			ball.x = STARTBALLX;
			ball.y = STARTBALLY;
			ball.sx = ballSpeed * 0.1;
			//The required speed in y direction to have the wanted ball speed
			ball.sy = ballSpeed * sqrtf((ballSpeed*ballSpeed) - (ball.sx * ball.sx));

			platform.x = PLATFORMSTARTX;
			platform.y = PLATFORMSTARTY;
			platform.sx = 0;
			platform.sy = 0;

			//Clear the ball
			clearRect(screen,(int)oldBallx,(int)oldBally,ballSizeX,ballSizeY);
			//draw the ball
			drawImage(screen,(int)ball.x,(int)ball.y,ballSizeX,ballSizeY,ballData, ballMask);

			//remove current position of platform
			clearRect(screen,(int)oldPlatformx,platform.y,platformSizeX,platformSizeY);
			//Draw the new platform
			drawImage(screen,(int)platform.x,(int)platform.y,platformSizeX,platformSizeY,platformData, NOMASK);

			//Refresh the whole screen. (Should not be required)
			refreshScreen(fd);
			//wait for a second
			usleep(1000000);

			//set the time such that the deltatime of the first frame will be 0
			startTime = getTime();
			//next frame
			continue;
		}

		//now repaint what we need to repaint
		if(collisionIndex >= 0 && level[collisionIndex] > 1)
		{
			//remove the block at the collision index
			level[collisionIndex] = 0;
			//we have one less block to remove
			removeCount--;

			//there are no more blocks to remove, and the level is done
			if(removeCount <= 0)
				return lives;

			//the position of the blocks in screen coordinates
			int blockPosX = (collisionIndex % BOARDSQUARESWIDE) * grayBlockSizeX;
			int blockPosY = (collisionIndex / BOARDSQUARESWIDE) * grayBlockSizeY;

			//clear the block
			clearRect(screen,blockPosX,blockPosY,grayBlockSizeX,grayBlockSizeY);

			//Refresh screen at the blocks position
			refreshRect(blockPosX,blockPosY,grayBlockSizeX,grayBlockSizeY,fd);
		}
		
		//the pixels position of where to draw the ball
		int drawBallx = (int)ball.x;
		int drawBally = (int)ball.y;

		//Clear the old ball on the screen
		clearRect(screen,(int)oldBallx,(int)oldBally,ballSizeX,ballSizeY);
		//draw the ball at the new position
		drawImage(screen,drawBallx,drawBally,ballSizeX,ballSizeY,ballData, ballMask);

		//refresh the screen at the ball position
		refreshRect(min(drawBallx,(int)oldBallx),		//the rect will include both the new and the old ball that is removed
			min(drawBally,(int)oldBally),
			ballSizeX + (int)absolute(drawBallx - oldBallx) + 1,	//the width must be the ball's dimension plus the ammount it has moved
			ballSizeY + (int)absolute(drawBally - oldBally) + 1, 
			fd);

		//We only need to paint and refresh the platform if it has moved
		if(platform.sx < -0.0001 || platform.sx > 0.0001)
		{
			//We clear the rect at the left of the platform
			if(platform.sx > 0)
				clearRect(screen,
					(int)oldPlatformx,	//Start at the old position
					(int)platform.y,
					(int)absolute((int)platform.x - (int)oldPlatformx),	//Clear the ammount it have moved
					platformSizeY);
			//we clear the rect at the right of the platform
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
		}

		long endTime = getTime();

		//sleep the remaining time of this frame.
		long sleepTime = SLEEPTIME - (endTime - startTime);

		/*long nextTime = endTime + sleepTime;
		if(sleepTime > 0)
			while(getTime() < nextTime);*/

		//sleep the remaining time of this frame if there is any time left.
		if(sleepTime > 0)
			usleep((int)sleepTime);
	}
	//we have no lives left when we have completed the game.
	return 0;
}

int main(int argc, char *argv[])
{
	//open the frame buffer driver
	int fd = open("/dev/fb0",O_RDWR);
	int level = 0;
	//the block of this level
	int * currentLevel;
	//set the default number of lives
	int gameResult = NUMBEROFLIVES;

	if(!fd) //if there was an errer open the file
		printf("File was not opened");
	//driver was successfully opened
	else
	{
		//an array of pixels representing the screen.
		uint16_t *screen = mmap(0,SCREENWIDTH*SCREENHEIGHT*2,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		//Clear the screen
		clearRect(screen,0,0,SCREENWIDTH,SCREENHEIGHT);

		//init the driver
		initDriver();

		//Go through all the levels.
		while(level < NUMBEROFLEVELS)
		{
			//set the current level
			currentLevel = loadMap(screen,level);

			//refresh the screen since the loadMap function have drawn to the screen
			refreshScreen(fd);

			//Play this level. gameResult will contain the number of lives left
			gameResult = playLevel(screen,currentLevel,gameResult,fd);
			
			//Free memory of current level
			free(currentLevel);

			//No lives left
			if(gameResult <= 0)
				break;	//stop the game loop

			//clear the screen such that 
			clearRect(screen,0,0,SCREENWIDTH,SCREENHEIGHT);

			//Next level
			level++;
			//sleep for a second
			usleep(1000000);
		}

		//The game is done. Eval result
		if(gameResult <= 0)
			printf("You lost the game :-(. Try again...\n");
		else
			printf("You won the game. That is impressive!\n");

		//close the drivers
		close(fd);
		close(driver_descriptor);
	}
	//exit program
	exit(EXIT_SUCCESS);
}
