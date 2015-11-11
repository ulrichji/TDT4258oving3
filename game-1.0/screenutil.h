#include <stdint.h>

#define SCREENWIDTH 320
#define SCREENHEIGHT 240

void drawImage(uint16_t *screen,int x,int y,int dx,int dy,const uint16_t *image);
void drawRect(uint16_t *screen,int x,int y,int dx,int dy,uint16_t color);
void clearRect(uint16_t *screen,int x,int y,int dx,int dy);
