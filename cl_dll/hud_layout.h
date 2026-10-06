#ifndef HUD_LAYOUT_H
#define HUD_LAYOUT_H

void HudLayout_Init( void );

int  HudLayout_Show( const char *element );
void HudLayout_Offset( const char *element, int *x, int *y );
void HudLayout_Color( int *r, int *g, int *b );
void HudLayout_ChatColor( float *r, float *g, float *b );

#endif
