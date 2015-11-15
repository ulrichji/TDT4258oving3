#include <stdint.h>

//size of screen
#define SCREENWIDTH 320
#define SCREENHEIGHT 240

//Different color mask. 
#define NOMASK (0xFFFF)
#define REDMASK (0x1F << 11)
#define GREENMASK (0x7E0)
#define BLUEMASK (0x1F)
#define YELLOWMASK (0xFFE0)

/* Draw an image on the screen at the specified area and given and given mask.
 * Screen is the screen to draw to
 * x and y is the position of the image
 * dx and dy is the size in x and y direction of the image
 * image is an array of colors that shall be copied to screen
 * colorMask is an integer representing a mask of wich color channels that shall be displayed */
void drawImage(uint16_t *screen,int x,int y,int dx,int dy,const uint16_t *image, uint16_t colorMask);

/* Draw a rectangle on the screen at the specified area and given color.
 * Screen is the screen to draw to
 * x and y is the position of the rectangle
 * dx and dy is the size in x and y direction of the rectangle to draw
 * color is the 16 bit integer representation of the color */
void drawRect(uint16_t *screen,int x,int y,int dx,int dy,uint16_t color);

/* Clear the screen at the specified area.
 * Screen is the screen that should be cleared from
 * x and y is the position of the rectangle
 * dx and dy is the size in x and y direction of the rectangle to clear*/
void clearRect(uint16_t *screen,int x,int y,int dx,int dy);

//Refresh the screen at position x,y and dimensions width,height.
//fd is the frame buffer that shall be refreshed
void refreshRect(int x,int y,int width,int height,int fd);

//Refreshing the whole screen.
void refreshScreen(int fd);
