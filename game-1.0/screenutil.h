#include <stdint.h>

#define SCREENWIDTH 320
#define SCREENHEIGHT 240

#define NOMASK (0xFFFF)
#define REDMASK (0x1F << 11)
#define GREENMASK (0x7E0)
#define BLUEMASK (0x1F)
#define YELLOWMASK (0xFFE0)

void drawImage(uint16_t *screen,int x,int y,int dx,int dy,const uint16_t *image, uint16_t colorMask);
void drawRect(uint16_t *screen,int x,int y,int dx,int dy,uint16_t color);
void clearRect(uint16_t *screen,int x,int y,int dx,int dy);
