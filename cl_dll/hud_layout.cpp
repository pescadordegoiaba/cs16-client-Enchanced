#include "hud.h"
#include "cl_util.h"
#include "hud_layout.h"

static cvar_t *show_health;
static cvar_t *show_armor;
static cvar_t *show_ammo;
static cvar_t *show_money;
static cvar_t *show_radar;
static cvar_t *show_timer;
static cvar_t *show_chat;
static cvar_t *show_death;

static cvar_t *dx_health, *dy_health;
static cvar_t *dx_armor, *dy_armor;
static cvar_t *dx_ammo, *dy_ammo;
static cvar_t *dx_money, *dy_money;
static cvar_t *dx_radar, *dy_radar;
static cvar_t *dx_timer, *dy_timer;
static cvar_t *dx_chat, *dy_chat;
static cvar_t *dx_death, *dy_death;

static cvar_t *hud_r, *hud_g, *hud_b;
static cvar_t *chat_r, *chat_g, *chat_b;

static cvar_t *Reg( const char *name, const char *value )
{
	return gEngfuncs.pfnRegisterVariable( name, value, FCVAR_ARCHIVE );
}

void HudLayout_Init( void )
{
	show_health = Reg( "hud_show_health", "1" );
	show_armor  = Reg( "hud_show_armor", "1" );
	show_ammo   = Reg( "hud_show_ammo", "1" );
	show_money  = Reg( "hud_show_money", "1" );
	show_radar  = Reg( "hud_show_radar", "1" );
	show_timer  = Reg( "hud_show_timer", "1" );
	show_chat   = Reg( "hud_show_chat", "1" );
	show_death  = Reg( "hud_show_death", "1" );

	dx_health = Reg( "hud_dx_health", "0" );
	dy_health = Reg( "hud_dy_health", "0" );
	dx_armor  = Reg( "hud_dx_armor", "0" );
	dy_armor  = Reg( "hud_dy_armor", "0" );
	dx_ammo   = Reg( "hud_dx_ammo", "0" );
	dy_ammo   = Reg( "hud_dy_ammo", "0" );
	dx_money  = Reg( "hud_dx_money", "0" );
	dy_money  = Reg( "hud_dy_money", "0" );
	dx_radar  = Reg( "hud_dx_radar", "0" );
	dy_radar  = Reg( "hud_dy_radar", "0" );
	dx_timer  = Reg( "hud_dx_timer", "0" );
	dy_timer  = Reg( "hud_dy_timer", "0" );
	dx_chat   = Reg( "hud_dx_chat", "0" );
	dy_chat   = Reg( "hud_dy_chat", "0" );
	dx_death  = Reg( "hud_dx_death", "0" );
	dy_death  = Reg( "hud_dy_death", "0" );

	hud_r = Reg( "hud_r", "255" );
	hud_g = Reg( "hud_g", "160" );
	hud_b = Reg( "hud_b", "0" );
	chat_r = Reg( "hud_chat_r", "255" );
	chat_g = Reg( "hud_chat_g", "210" );
	chat_b = Reg( "hud_chat_b", "0" );
}

static cvar_t *ShowCvar( const char *element )
{
	if( !strcmp( element, "health" ) ) return show_health;
	if( !strcmp( element, "armor" ) ) return show_armor;
	if( !strcmp( element, "ammo" ) ) return show_ammo;
	if( !strcmp( element, "money" ) ) return show_money;
	if( !strcmp( element, "radar" ) ) return show_radar;
	if( !strcmp( element, "timer" ) ) return show_timer;
	if( !strcmp( element, "chat" ) ) return show_chat;
	if( !strcmp( element, "death" ) ) return show_death;
	return NULL;
}

int HudLayout_Show( const char *element )
{
	cvar_t *cv = ShowCvar( element );
	if( !cv )
		return 1;
	return cv->value != 0.0f;
}

void HudLayout_Offset( const char *element, int *x, int *y )
{
	cvar_t *dx = NULL;
	cvar_t *dy = NULL;

	if( !strcmp( element, "health" ) ) { dx = dx_health; dy = dy_health; }
	else if( !strcmp( element, "armor" ) ) { dx = dx_armor; dy = dy_armor; }
	else if( !strcmp( element, "ammo" ) ) { dx = dx_ammo; dy = dy_ammo; }
	else if( !strcmp( element, "money" ) ) { dx = dx_money; dy = dy_money; }
	else if( !strcmp( element, "radar" ) ) { dx = dx_radar; dy = dy_radar; }
	else if( !strcmp( element, "timer" ) ) { dx = dx_timer; dy = dy_timer; }
	else if( !strcmp( element, "chat" ) ) { dx = dx_chat; dy = dy_chat; }
	else if( !strcmp( element, "death" ) ) { dx = dx_death; dy = dy_death; }

	if( x && dx )
		*x += (int)dx->value;
	if( y && dy )
		*y += (int)dy->value;
}

static int ClampByte( cvar_t *cv, int fallback )
{
	if( !cv )
		return fallback;
	int v = (int)cv->value;
	if( v < 0 ) v = 0;
	if( v > 255 ) v = 255;
	return v;
}

void HudLayout_Color( int *r, int *g, int *b )
{
	if( r ) *r = ClampByte( hud_r, 255 );
	if( g ) *g = ClampByte( hud_g, 160 );
	if( b ) *b = ClampByte( hud_b, 0 );
}

void HudLayout_ChatColor( float *r, float *g, float *b )
{
	if( r ) *r = ClampByte( chat_r, 255 ) / 255.0f;
	if( g ) *g = ClampByte( chat_g, 210 ) / 255.0f;
	if( b ) *b = ClampByte( chat_b, 0 ) / 255.0f;
}
