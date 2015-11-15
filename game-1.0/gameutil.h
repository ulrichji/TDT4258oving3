
//Returns the absolute value of x
float absolute(float x);

//Returns the square root of x.
//it is optimized for square root of 10
float sqrtf(float x);

//Returns the smallest of a and b
int min(int a,int b);

//Returns the largest of a and b
int max(int a,int b);

//Returns the microsecond of this day.
long getTime();

//A struct that represents an object in the game that is movable
struct MovableGameObject
{
	float x;		//the x position
	float y;		//the y position
	float sx;		//the speed in x direction
	float sy;		//the speed in y direction
	int width;		//the width of the object
	int height;		//the height of the object
};