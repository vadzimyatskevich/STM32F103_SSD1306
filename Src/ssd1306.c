/**
 *  \file ssd1306.c
 *  \brief Brief
 */

#include <string.h>
#include <stdlib.h>
#include "ssd1306.h"
#include "font5x7.h"
#include "mxconstants.h"

#define CMD(c)        do { HAL_GPIO_WritePin( DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET); \
                           ssd1306SendByte( c ); \
                         } while (0);
#define DATA(c)       do { HAL_GPIO_WritePin( DC_GPIO_Port, DC_Pin, GPIO_PIN_SET); \
                           ssd1306SendByte( c ); \
                         } while (0); 
#define swap(a, b)       { int16_t t = a; a = b; b = t; }
												 
extern SPI_HandleTypeDef hspi2;										 

static uint8_t    buffer[SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8];

#ifdef MULTILAYER  
static uint8_t    buffer_ol[SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8];
#endif    

static uint8_t   _width    = SSD1306_LCDWIDTH;
static uint8_t   _height   = SSD1306_LCDHEIGHT;


/**
 *  \brief SPI write
 *  
 *  \param [in] byte Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 *  
 *  \code
 *  \endcode
 */
inline void ssd1306SendByte(uint8_t byte)
{
	HAL_SPI_Transmit  ( &hspi2,  (uint8_t *) &byte, 1, 20) ;
}

/**************************************************************************/
/**
 *  \brief Initialises the SSD1306 LCD display
 *  
 *  \param [in] vccstate Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void  ssd1306Init(uint8_t vccstate)
{

  HAL_Delay  (100);
  // Initialisation sequence
  CMD(SSD1306_DISPLAYOFF);                    // 0xAE
  CMD(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
  CMD(0x80);                                  // the suggested ratio 0x80
  CMD(SSD1306_SETMULTIPLEX);                  // 0xA8
  CMD(0x3F);
  CMD(SSD1306_SETDISPLAYOFFSET);              // 0xD3
  CMD(0x0);                                   // no offset
  CMD(SSD1306_SETSTARTLINE | 0x0);            // line #0
  CMD(SSD1306_CHARGEPUMP);                    // 0x8D
  if (vccstate == SSD1306_EXTERNALVCC) 
  { CMD(0x10); }
  else 
  { CMD(0x14); }
  CMD(SSD1306_MEMORYMODE);                    // 0x20
  CMD(0x00);                                  // 0x0 act like ks0108
  CMD(SSD1306_SEGREMAP | 0x1);
  CMD(SSD1306_COMSCANDEC);
  CMD(SSD1306_SETCOMPINS);                    // 0xDA
  CMD(0x12);
  CMD(SSD1306_SETCONTRAST);                   // 0x81
  if (vccstate == SSD1306_EXTERNALVCC) 
  { CMD(0x9F); }
  else 
  { CMD(0xCF); }
  CMD(SSD1306_SETPRECHARGE);                  // 0xd9
  if (vccstate == SSD1306_EXTERNALVCC) 
  { CMD(0x22); }
  else 
  { CMD(0xF1); }
  CMD(SSD1306_SETVCOMDETECT);                 // 0xDB
  CMD(0x40);
  CMD(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
  CMD(SSD1306_NORMALDISPLAY);                 // 0xA6
  HAL_Delay  (100);
  ssd1306TurnOn();
}

/**
 *  \brief Renders the contents of the pixel buffer on the LCD
 *  
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306Refresh(void) 
{
  CMD(SSD1306_SETLOWCOLUMN  | 0x0);  // low col = 0
  CMD(SSD1306_SETHIGHCOLUMN | 0x0);  // hi col = 0
  CMD(SSD1306_SETSTARTLINE  | 0x0);  // line #0

  HAL_GPIO_WritePin( DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
  #ifdef MULTILAYER
  ssd1306MixFrameBuffer();
  HAL_SPI_Transmit  ( &hspi2,  (uint8_t *) buffer_ol, sizeof(buffer), 200) ;
  #else
  HAL_SPI_Transmit  ( &hspi2,  (uint8_t *) buffer, sizeof(buffer), 200) ;
  #endif

}

/**
 *  \brief Enable the OLED panel
 *  
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306TurnOn(void)
{
    CMD(SSD1306_DISPLAYON);
}
/**
 *  \brief Disable the OLED panel
 *  
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306TurnOff(void)
{
    CMD(SSD1306_DISPLAYOFF);
}

/**************************************************************************/
/*! 
    @brief Draws a single pixel in image buffer
    @param[in]  x
                The x position (0..127)
    @param[in]  y
                The y position (0..63)
*/
/**************************************************************************/
void   ssd1306DrawPixel(int16_t x, int16_t y, uint16_t color, uint16_t layer) 
{
  if ((x >= SSD1306_LCDWIDTH) || (x < 0) || (y >= SSD1306_LCDHEIGHT) || (y < 0))
  return;

  if (layer & LAYER0)
  switch (color) 
  {
  case WHITE:   buffer[x+ (y/8)*SSD1306_LCDWIDTH] |=  (1 << (y&7)); break;
  case BLACK:   buffer[x+ (y/8)*SSD1306_LCDWIDTH] &= ~(1 << (y&7)); break; 
  case INVERSE: buffer[x+ (y/8)*SSD1306_LCDWIDTH] ^=  (1 << (y&7)); break; 
  }
#ifdef MULTILAYER  
  if (layer & LAYER1)
  switch (color) 
  {
  case WHITE:   buffer_ol[x+ (y/8)*SSD1306_LCDWIDTH] |=  (1 << (y&7)); break;
  case BLACK:   buffer_ol[x+ (y/8)*SSD1306_LCDWIDTH] &= ~(1 << (y&7)); break; 
  case INVERSE: buffer_ol[x+ (y/8)*SSD1306_LCDWIDTH] ^=  (1 << (y&7)); break; 
  }
#endif
//	ssd1306Refresh();
}

/**************************************************************************/
/*! 
    @brief Clears a single pixel in image buffer
    @param[in]  x
                The x position (0..127)
    @param[in]  y
                The y position (0..63)
*/
/**************************************************************************/
//void ssd1306ClearPixel(int16_t x, int16_t y) 
//{
//  if ((x >= SSD1306_LCDWIDTH) || (x < 0) || (y >= SSD1306_LCDHEIGHT) || (y < 0))
//    return;

//  buffer[x+ (y/8)*SSD1306_LCDWIDTH] &= ~(1 << y%8); 
//}

/**************************************************************************/
/*! 
    @brief Gets the value (1 or 0) of the specified pixel from the buffer
    @param[in]  x
                The x position (0..127)
    @param[in]  y
                The y position (0..63)
    @return     1 if the pixel is enabled, 0 if disabled
*/
/**************************************************************************/
uint8_t ssd1306GetPixel(int16_t x, int16_t y)
{
  if ((x >= SSD1306_LCDWIDTH) || (x < 0) || (y >= SSD1306_LCDHEIGHT) || (y < 0)) return 0;

  return buffer[x+ (y/8)*SSD1306_LCDWIDTH] & (1 << y%8) ? 1 : 0;
}

/**
 *  \brief Clears the screen
 *  
 *  \param [in] layer Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306ClearScreen(uint16_t layer) 
{
  if (layer & 0x1)
  memset(buffer, 0x00, 1024);
#ifdef MULTILAYER  
  if (layer & 0x2)
  memset(buffer_ol, 0x00, 1024);
#endif
}

/**
 *  \brief Brief
 *  
 *  \param [in] x0 Parameter_Description
 *  \param [in] y0 Parameter_Description
 *  \param [in] x1 Parameter_Description
 *  \param [in] y1 Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color, uint16_t layer) {
  int16_t steep = (abs(y1 - y0) > abs(x1 - x0));
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx            = x1 - x0;
  dy            = abs(y1 - y0);

  int16_t err   = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep       = 1;
  } else {
    ystep       = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      ssd1306DrawPixel(y0, x0, color, layer);
    } else {
      ssd1306DrawPixel(x0, y0, color, layer);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}
/**
 *  \brief Brief
 *  
 *  \param [in] SSD1306_point Parameter_Description
 *  \param [in] count Parameter_Description
 *  \param [in] x Parameter_Description
 *  \param [in] y Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 *  
 *  \code
 *  \endcode
 */
void ssd1306_DrawPolygon(SSD1306_polyTypeDef * SSD1306_poly, int16_t x, int16_t y, uint16_t color, uint16_t layer)
{
	int16_t i;
	
	if (SSD1306_poly->SSD1306_points_number < 2) return;
	for(i = 0; i < (SSD1306_poly->SSD1306_points_number - 1); i++)
	{
		ssd1306DrawLine(round(SSD1306_poly->SSD1306_points_pointer[i].x + x), round(SSD1306_poly->SSD1306_points_pointer[i].y + y), round(SSD1306_poly->SSD1306_points_pointer[i+1].x + x), round(SSD1306_poly->SSD1306_points_pointer[i+1].y + y), color, layer);
	}
	ssd1306DrawLine(round(SSD1306_poly->SSD1306_points_pointer[i].x + x), round(SSD1306_poly->SSD1306_points_pointer[i].y + y), round(SSD1306_poly->SSD1306_points_pointer[0].x + x), round(SSD1306_poly->SSD1306_points_pointer[0].y + y), color, layer);
}

/**
 *  \brief Fill poly http://alienryderflex.com/polygon_fill/
 *  
 *  \param [in] SSD1306_point Parameter_Description
 *  \param [in] count Parameter_Description
 *  \param [in] x Parameter_Description
 *  \param [in] y Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 *  
 *  \code
 *  \endcode
 */
void ssd1306_FillPolygon(SSD1306_polyTypeDef * SSD1306_poly, double x, double y, uint16_t color, uint16_t layer)
{
	int     nodes, nodeX[MAX_POLY_CORNERS], i, j, swap ;
  double  polyX[MAX_POLY_CORNERS], polyY[MAX_POLY_CORNERS], pixelX, pixelY;
	
	// draw shape
	ssd1306_DrawPolygon(SSD1306_poly, x, y, color, layer);
	// copy poly 
	i = SSD1306_poly->SSD1306_points_number - 1;
	while (i >= 0)
	{
		polyX[i] = round(SSD1306_poly->SSD1306_points_pointer[i].x + x);
		polyY[i] = round(SSD1306_poly->SSD1306_points_pointer[i].y + y);
		i--;
	}
	//  Loop through the rows of the image.
	for (pixelY = 0; pixelY < SSD1306_LCDHEIGHT; pixelY++) {
		//  Build a list of nodes.
		nodes = 0;
		j = SSD1306_poly->SSD1306_points_number - 1;
		for (i=0; i<SSD1306_poly->SSD1306_points_number; i++) {
			if ((polyY[i]<(double) pixelY && polyY[j]>=(double) pixelY)	
			||  (polyY[j]<(double) pixelY && polyY[i]>=(double) pixelY)) {
				nodeX[nodes++]=(int)round(polyX[i]+(pixelY-polyY[i])/(polyY[j]-polyY[i]) * (polyX[j]-polyX[i])); 
			}
			j=i; 
		}
		//  Sort the nodes, via a simple “Bubble” sort.
		i=0;
		while (i<nodes-1) {
			if (nodeX[i]>nodeX[i+1]) {
				swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; 
			}	else {
				i++; 
			}
		}
		//  Fill the pixels between node pairs.
		for (i=0; i<nodes; i+=2) {
			if   (nodeX[i  ]>=SSD1306_LCDWIDTH) break;
			if   (nodeX[i+1]> 0 ) {
				if (nodeX[i  ]< 0 ) nodeX[i  ]=0 ;
				if (nodeX[i+1]> SSD1306_LCDWIDTH) nodeX[i+1]=SSD1306_LCDWIDTH;
				for (pixelX=nodeX[i]; pixelX<=nodeX[i+1]; pixelX++) {
					ssd1306DrawPixel((pixelX), (pixelY), color, layer);
				} 
			}
		}
	}
}


/**
 *  \brief Brief
 *  
 *  \param [in] x Parameter_Description
 *  \param [in] y Parameter_Description
 *  \param [in] h Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306DrawFastVLine(int16_t x, int16_t y,
				 int16_t h, uint16_t color, uint16_t layer) {

  ssd1306DrawLine(x, y, x, y+h-1, color, layer);
}

/**
 *  \brief Brief
 *  
 *  \param [in] x Parameter_Description
 *  \param [in] y Parameter_Description
 *  \param [in] w Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306DrawFastHLine(int16_t x, int16_t y,
				 int16_t w, uint16_t color, uint16_t layer) {
					 
  ssd1306DrawLine(x, y, x+w-1, y, color, layer);
}

/**
 *  \brief Brief
 *  
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306FillScreen(uint16_t color) {
	
  ssd1306FillRect(0, 0, _width, _height, color, LAYER0);
}

/**
 *  \brief Draws a circle, midpoint circle algorithm
 *  
 *  \param [in] x0 Parameter_Description
 *  \param [in] y0 Parameter_Description
 *  \param [in] radius Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color) {
  int x = radius;
  int y = 0;
  int decisionOver2 = 1 - x;   // Decision criterion divided by 2 evaluated at x=r, y=0

  if ( x < 0) return;
  while( y <= x )
  {
    ssd1306DrawPixel( x + x0,  y + y0, color, LAYER0); // Octant 1
    ssd1306DrawPixel( y + x0,  x + y0, color, LAYER0); // Octant 2
    ssd1306DrawPixel(-x + x0,  y + y0, color, LAYER0); // Octant 4
    ssd1306DrawPixel(-y + x0,  x + y0, color, LAYER0); // Octant 3
    ssd1306DrawPixel(-x + x0, -y + y0, color, LAYER0); // Octant 5
    ssd1306DrawPixel(-y + x0, -x + y0, color, LAYER0); // Octant 6
    ssd1306DrawPixel( x + x0, -y + y0, color, LAYER0); // Octant 8
    ssd1306DrawPixel( y + x0, -x + y0, color, LAYER0); // Octant 7
    y++;
    if (decisionOver2<=0)
    {
      decisionOver2 += 2 * y + 1;   // Change in decision criterion for y -> y+1
    }
    else
    {
      x--;
      decisionOver2 += 2 * (y - x) + 1;   // Change for y -> y+1, x -> x-1
    }
  }
}

/**
 *  \brief Draw circle helper
 *  
 *  \param [in] x0 Parameter_Description
 *  \param [in] y0 Parameter_Description
 *  \param [in] r Parameter_Description
 *  \param [in] cornername Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void ssd1306DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color, uint16_t layer) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      ssd1306DrawPixel(x0 + x, y0 + y, color, layer);
      ssd1306DrawPixel(x0 + y, y0 + x, color, layer);
    } 
    if (cornername & 0x2) {
      ssd1306DrawPixel(x0 + x, y0 - y, color, layer);
      ssd1306DrawPixel(x0 + y, y0 - x, color, layer);
    }
    if (cornername & 0x8) {
      ssd1306DrawPixel(x0 - y, y0 + x, color, layer);
      ssd1306DrawPixel(x0 - x, y0 + y, color, layer);
    }
    if (cornername & 0x1) {
      ssd1306DrawPixel(x0 - y, y0 - x, color, layer);
      ssd1306DrawPixel(x0 - x, y0 - y, color, layer);
    }
  }
}

/**
 *  \brief Brief
 *  
 *  \param [in] x0 Parameter_Description
 *  \param [in] y0 Parameter_Description
 *  \param [in] r Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *  
 *  \details Details
 */
void    ssd1306FillCircle(int16_t x0, int16_t y0, int16_t r,uint16_t color, uint16_t layer) {
  ssd1306DrawFastVLine(x0, y0-r, 2*r+1, color, layer);
  ssd1306FillCircleHelper(x0, y0, r, 3, 0, color, layer);
}

/**************************************************************************/
/*!
    @brief Used to do circles and roundrects
*/
/**************************************************************************/
void ssd1306FillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color, uint16_t layer) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      ssd1306DrawFastVLine(x0+x, y0-y, 2*y+1+delta, color, layer);
      ssd1306DrawFastVLine(x0+y, y0-x, 2*x+1+delta, color, layer);
    }
    if (cornername & 0x2) {
      ssd1306DrawFastVLine(x0-x, y0-y, 2*y+1+delta, color, layer);
      ssd1306DrawFastVLine(x0-y, y0-x, 2*x+1+delta, color, layer);
    }
  }
}
/**************************************************************************/
/*!
    @brief Draws a filled rectangle
*/
/**************************************************************************/
void ssd1306FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color, uint16_t layer) {
	uint8_t x0, x1, y1;

	x0 = x;
	x1 = x + w;
	y1 = y + h;
	for(; y < y1; y++)
			for(x = x0; x < x1; x++)
					ssd1306DrawPixel( x, y, color, layer);
}

/**************************************************************************/
/*!
    @brief Draws a rectangle
*/
/**************************************************************************/
void ssd1306DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint16_t layer) {
	int16_t x1, y1;
	if ( (w == 0) | (h == 0)) return;
	x1 = x + w - 1;
	y1 = y + h - 1;
	
	if((h > 2 ) | (w > 2)) {
		ssd1306DrawLine(	 x,    y,   x1,  y, color, layer);
		ssd1306DrawLine(	 x,   y1,   x1, y1, color, layer);
		ssd1306DrawLine(	 x,  y+1,  x, y1-1, color, layer);
		ssd1306DrawLine(  x1,  y+1, x1, y1-1, color, layer);
	} else {
		ssd1306DrawLine(	 x,    y,   x1,  y, color, layer);
		ssd1306DrawLine(	 x,   y1,   x1, y1, color, layer);
	}
}

/**************************************************************************/
/*!
    @brief Draws a filled rectangle
*/
/**************************************************************************/
void ssd1306DrawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

  uint8_t *pbitmap;
  int16_t i, j, byteWidth = (w + 7) / 8;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      pbitmap = bitmap + j * byteWidth + i / 8;
      if((uint8_t)(*pbitmap) & (128 >> (i & 7))) {
        ssd1306DrawPixel(x+i, y+j, color, LAYER0);
      }
    }
  }
}

/**************************************************************************/
/*! 
    @brief Draw a character
*/
/**************************************************************************/
void  ssd1306DrawChar(int16_t x, int16_t y, uint8_t c, uint8_t size, 
uint16_t color, uint16_t layer) {

  if( (x >= _width)            || // Clip right
      (y >= _height)           || // Clip bottom
      ((x + 6 * size - 1) < 0) || // Clip left
      ((y + 8 * size - 1) < 0))   // Clip top
  return;

  for (int8_t i=0; i<6; i++ ) {
    int8_t line;
    if (i == 5) 
    line = 0x0;
    else 
    line = (int8_t)*(font5x7+(c*5)+i);
    for (int8_t j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
        ssd1306DrawPixel(x+i, y+j, color, layer);
        else {  // big size
          ssd1306DrawRect(x+(i*size), y+(j*size), size, size, color, layer);
        } 
      } 
      line >>= 1;
    }
  }
}

/**************************************************************************/
/*!
    @brief  Draws a string using the supplied font data.
    @param[in]  x
                Starting x co-ordinate
    @param[in]  y
                Starting y co-ordinate
    @param[in]  text
                The string to render
    @param[in]  font
                Pointer to the FONT_DEF to use when drawing the string
    @section Example
    @code 
    #include "drivers/lcd/bitmap/ssd1306/ssd1306.h"
    #include "drivers/lcd/smallfonts.h"
    
    // Configure the pins and initialise the LCD screen
    ssd1306Init();
    // Render some text on the screen
    ssd1306DrawString(1, 10, "5x8 System", Font_System5x8);
    ssd1306DrawString(1, 20, "7x8 System", Font_System7x8);
    // Refresh the screen to see the results
    ssd1306Refresh();
    @endcode
*/
/**************************************************************************/
void  ssd1306DrawString(int16_t x, int16_t y, const char *text, uint8_t size, 
                        uint16_t color, uint16_t layer)
{
  uint8_t l;
  for (l = 0; l < strlen(text); l++)
  {
    ssd1306DrawChar(x + (l * (5*size + 1)), y, text[l], size, color, layer);
  }
}

/**************************************************************************/
/*!
    @brief  Shifts the contents of the frame buffer up the specified
            number of pixels
    @param[in]  height
                The number of pixels to shift the frame buffer up, leaving
                a blank space at the bottom of the frame buffer x pixels
                high
*/
/**************************************************************************/
void ssd1306ShiftFrameBuffer( uint16_t height, uint16_t direction)
{  
  int16_t y, x;
  if (height == 0) return;
//while (height > 0)
//{
  switch (direction)
  {	
    case 0:
    // This is horribly inefficient, but at least easy to understand
    // In a production environment, this should be significantly optimised
    if (height >= SSD1306_LCDHEIGHT)
    {
      // Clear the entire frame buffer
      ssd1306ClearScreen( LAYER0 );
      return;
    }
    for (y = 0; y < SSD1306_LCDHEIGHT; y++)
    {
      for (x = 0; x < SSD1306_LCDWIDTH; x++)
      {
        if ((SSD1306_LCDHEIGHT ) - y > 1)
        {
          // Shift height from further ahead in the buffer
          ssd1306GetPixel(x, y + 1) ? ssd1306DrawPixel(x, y, WHITE, LAYER0) : ssd1306DrawPixel(x, y, BLACK, LAYER0);
        }
        else
        {
          // Clear the entire line
          ssd1306DrawPixel(x, y, BLACK, LAYER0);
        }
      }
    }
    break;
    case 1:
    {
    //static uint8_t    buffer[SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8];
      y = SSD1306_COUNTOF(buffer);
      x = y;
      for (x = SSD1306_COUNTOF(buffer) - 1; x >= 0 /*SSD1306_COUNTOF(buffer)*/; x--)
      {
        buffer[x] = buffer[x] << height;
        if (((x - (int16_t)SSD1306_COUNTOF(buffer)/8) >= 0) && ((buffer[x - SSD1306_COUNTOF(buffer)/8] & 0x80) == 0x80))
        {
          buffer[x] = buffer[x] | 0x01;
        }
      }
    }
    break;
    case 2:
    {
      for(y = 0; y <= 7; y++)
      {
        for (x = 0; x < (SSD1306_COUNTOF(buffer)/8 - 1); x++)
        {
          buffer[x + (128 * y)] = buffer[x + (128 * y) + 1];
        }
        buffer[127 + (128 * y)] = 0;
      }
    }
    break;
    case 3:
    {
      for(y = 0; y <= 7; y++)
      {
        for (x = (SSD1306_COUNTOF(buffer)/8 - 1); x > 0; x--)
        {
          buffer[x + (128 * y)] = buffer[x - 1 + (128 * y)];
        }
        buffer[128 * y] = 0;
      }
    }
    break;
    default: return;
    break;
  }
//  height--;
//  }
}

/**
 *  \brief Brief
 *  
 *  \return Return_Description
 *  
 *  \details Details
 */
#ifdef MULTILAYER  
void ssd1306MixFrameBuffer(void)
{
  uint16_t i;
  for(i = 0; i < (SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8); i++)
  {
    buffer_ol[i] |= buffer[i];
  }
#endif    
}
