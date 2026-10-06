#include <math.h>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "pm_defs.h"
#include "in_buttons.h"
#include "keydefs.h"
#include "draw_util.h"
#include "aimbot.h"

extern int g_iTeamNumber;
extern int g_iUser1;
extern cvar_t *cl_aim_assist;
extern cvar_t *cl_aim_smooth;
extern cvar_t *cl_aim_fov;

void VectorAngles( const float *forward, float *angles );

static cvar_t *cl_aim_attack;
static cvar_t *cl_aim_head;
static cvar_t *cl_aim_chest;
static cvar_t *cl_aim_body;

static int g_menu;

#define AIM_BOX_MAX 384
static aim_box_t g_boxes[AIM_BOX_MAX];
static int g_boxCount;

static const float kFovSteps[] = { 10.f, 20.f, 35.f, 60.f, 90.f, 180.f };
static const float kSmoothSteps[] = { 0.15f, 0.35f, 0.65f, 1.f };

/* Trava offline removida: aimbot funciona em qualquer sessão. */
static int On( cvar_t *cv, int fallback )
{
	if( !cv )
		return fallback;
	return cv->value > 0.5f;
}

static void ToggleCvar( cvar_t *cv )
{
	if( !cv || !cv->name )
		return;
	gEngfuncs.Cvar_SetValue( cv->name, cv->value > 0.5f ? 0.f : 1.f );
}

static void CycleCvar( cvar_t *cv, const float *steps, int n )
{
	int i, best;
	float dist;

	if( !cv || !cv->name || n < 1 )
		return;

	best = 0;
	dist = 100000.f;
	for( i = 0; i < n; i++ )
	{
		float d = fabsf( cv->value - steps[i] );
		if( d < dist )
		{
			dist = d;
			best = i;
		}
	}
	gEngfuncs.Cvar_SetValue( cv->name, steps[( best + 1 ) % n] );
}

static void Cmd_AimbotMenu( void )
{
	g_menu = !g_menu;
}

void Aimbot_Init( void )
{
	cl_aim_attack = gEngfuncs.pfnRegisterVariable( "cl_aim_attack", "0", FCVAR_ARCHIVE );
	cl_aim_head = gEngfuncs.pfnRegisterVariable( "cl_aim_head", "1", FCVAR_ARCHIVE );
	cl_aim_chest = gEngfuncs.pfnRegisterVariable( "cl_aim_chest", "1", FCVAR_ARCHIVE );
	cl_aim_body = gEngfuncs.pfnRegisterVariable( "cl_aim_body", "1", FCVAR_ARCHIVE );
	gEngfuncs.pfnAddCommand( "aimbot_menu", Cmd_AimbotMenu );
}

void Aimbot_SetBoxes( const aim_box_t *boxes, int count )
{
	if( !boxes || count <= 0 )
	{
		g_boxCount = 0;
		return;
	}
	if( count > AIM_BOX_MAX )
		count = AIM_BOX_MAX;
	memcpy( g_boxes, boxes, sizeof( aim_box_t ) * count );
	g_boxCount = count;
}

static int EnemyAlive( int index )
{
	cl_entity_t *local;
	cl_entity_t *ent;
	int team;

	local = gEngfuncs.GetLocalPlayer();
	if( !local || index <= 0 || index == local->index )
		return 0;
	if( index > gEngfuncs.GetMaxClients() )
		return 0;

	ent = gEngfuncs.GetEntityByIndex( index );
	if( !ent || !ent->player || !ent->model )
		return 0;
	if( g_PlayerExtraInfo[index].dead )
		return 0;

	team = g_PlayerExtraInfo[index].teamnumber;
	if( team != 1 && team != 2 )
		return 0;
	if( g_iTeamNumber != 0 && team == g_iTeamNumber )
		return 0;
	return 1;
}

static int PointVisible( const float *eye, const float *point )
{
	float start[3], end[3];
	pmtrace_t tr;

	start[0] = eye[0];
	start[1] = eye[1];
	start[2] = eye[2];
	end[0] = point[0];
	end[1] = point[1];
	end[2] = point[2];
	gEngfuncs.pEventAPI->EV_PlayerTrace( start, end, PM_WORLD_ONLY, -1, &tr );
	return !tr.startsolid && tr.fraction >= 0.97f;
}

static float AngleTo( const float *eye, const float *point, const float *view, float *aim )
{
	float delta[3];
	float dy, dp;

	delta[0] = point[0] - eye[0];
	delta[1] = point[1] - eye[1];
	delta[2] = point[2] - eye[2];
	VectorAngles( delta, aim );
	if( aim[0] > 180.f )
		aim[0] -= 360.f;
	aim[0] = -aim[0];

	dy = aim[1] - view[1];
	while( dy > 180.f )
		dy -= 360.f;
	while( dy < -180.f )
		dy += 360.f;
	dp = aim[0] - view[0];
	return sqrtf( dy * dy + dp * dp );
}

static int ZoneEnabled( int group )
{
	if( group == 1 )
		return On( cl_aim_head, 1 );
	if( group == 2 )
		return On( cl_aim_chest, 1 );
	return On( cl_aim_body, 1 );
}

static int ZoneRank( int group )
{
	if( group == 1 )
		return 0;
	if( group == 2 )
		return 1;
	return 2;
}

int Aimbot_Apply( float viewangles[3], int buttons )
{
	float eye[3], ofs[3];
	float bestAim[3];
	float bestAngle;
	int bestRank;
	int i;
	cl_entity_t *local;
	float fov, smooth, dy, dp;

	/* Trava offline removida: sem OfflineBots() aqui. */
	if( !On( cl_aim_assist, 0 ) )
		return 0;
	if( g_iUser1 != 0 )
		return 0;
	if( On( cl_aim_attack, 0 ) && !( buttons & IN_ATTACK ))
		return 0;
	if( g_boxCount <= 0 )
		return 0;

	local = gEngfuncs.GetLocalPlayer();
	if( !local )
		return 0;

	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( ofs );
	eye[0] = local->origin[0] + ofs[0];
	eye[1] = local->origin[1] + ofs[1];
	eye[2] = local->origin[2] + ofs[2];

	fov = cl_aim_fov ? cl_aim_fov->value : 35.f;
	if( fov < 1.f )
		fov = 1.f;

	bestRank = 3;
	bestAngle = fov;
	bestAim[0] = viewangles[0];
	bestAim[1] = viewangles[1];
	bestAim[2] = 0.f;

	for( i = 0; i < g_boxCount; i++ )
	{
		float aim[3];
		float ang;
		int rank;

		if( !ZoneEnabled( g_boxes[i].group ))
			continue;
		if( !EnemyAlive( g_boxes[i].player ))
			continue;
		if( !PointVisible( eye, g_boxes[i].origin ))
			continue;

		rank = ZoneRank( g_boxes[i].group );
		ang = AngleTo( eye, g_boxes[i].origin, viewangles, aim );
		if( ang > fov )
			continue;
		if( rank > bestRank )
			continue;
		if( rank < bestRank || ang < bestAngle )
		{
			bestRank = rank;
			bestAngle = ang;
			bestAim[0] = aim[0];
			bestAim[1] = aim[1];
			bestAim[2] = 0.f;
		}
	}

	if( bestRank > 2 )
		return 0;

	smooth = cl_aim_smooth ? cl_aim_smooth->value : 0.35f;
	if( smooth < 0.05f )
		smooth = 0.05f;
	if( smooth > 1.f )
		smooth = 1.f;

	dy = bestAim[1] - viewangles[1];
	while( dy > 180.f )
		dy -= 360.f;
	while( dy < -180.f )
		dy += 360.f;
	dp = bestAim[0] - viewangles[0];

	viewangles[0] += dp * smooth;
	viewangles[1] += dy * smooth;
	viewangles[2] = 0.f;
	if( viewangles[0] > 89.f )
		viewangles[0] = 89.f;
	if( viewangles[0] < -89.f )
		viewangles[0] = -89.f;
	return 1;
}

static void DrawLine( int x, int y, const char *text, int r, int g, int b )
{
	DrawUtils::DrawHudString( x, y, ScreenWidth - 8, text, r, g, b );
}

void Aimbot_Draw( void )
{
	int x, y, w, row;
	char buf[96];

	if( !g_menu )
	{
		/* Sem trava: só mostra indicador quando o aimbot está ligado. */
		if( On( cl_aim_assist, 0 ) )
			DrawLine( XRES( 8 ), YRES( 8 ), "AIM", 255, 220, 80 );
		return;
	}

	x = XRES( 12 );
	y = YRES( 40 );
	w = XRES( 250 );
	gEngfuncs.pfnFillRGBA( x - 6, y - 8, w, YRES( 176 ), 0, 0, 0, 170 );
	gEngfuncs.pfnFillRGBA( x - 6, y - 8, w, 2, 255, 176, 32, 255 );

	DrawLine( x, y, "Aimbot  (F6 ou aimbot_menu)", 255, 200, 80 );
	row = y + YRES( 16 );

	snprintf( buf, sizeof( buf ), "1  Ligado          %s", On( cl_aim_assist, 0 ) ? "sim" : "nao" );
	DrawLine( x, row, buf, 255, 255, 255 );
	row += YRES( 14 );
	snprintf( buf, sizeof( buf ), "2  FOV             %.0f", cl_aim_fov ? cl_aim_fov->value : 0.f );
	DrawLine( x, row, buf, 255, 255, 255 );
	row += YRES( 14 );
	snprintf( buf, sizeof( buf ), "3  Suavidade       %.2f", cl_aim_smooth ? cl_aim_smooth->value : 0.f );
	DrawLine( x, row, buf, 255, 255, 255 );
	row += YRES( 14 );
	snprintf( buf, sizeof( buf ), "4  So ao atirar    %s", On( cl_aim_attack, 0 ) ? "sim" : "nao" );
	DrawLine( x, row, buf, 255, 255, 255 );
	row += YRES( 14 );
	snprintf( buf, sizeof( buf ), "5  Cabeca          %s", On( cl_aim_head, 1 ) ? "sim" : "nao" );
	DrawLine( x, row, buf, 255, 255, 255 );
	row += YRES( 14 );
	snprintf( buf, sizeof( buf ), "6  Peito           %s", On( cl_aim_chest, 1 ) ? "sim" : "nao" );
	DrawLine( x, row, buf, 255, 255, 255 );
	row += YRES( 14 );
	snprintf( buf, sizeof( buf ), "7  Partes visiveis %s", On( cl_aim_body, 1 ) ? "sim" : "nao" );
	DrawLine( x, row, buf, 255, 255, 255 );
	row += YRES( 16 );
	DrawLine( x, row, "sessao: livre (sem trava offline)", 180, 180, 180 );
	row += YRES( 14 );
	DrawLine( x, row, "0  Fechar", 200, 200, 200 );
}

int Aimbot_Key( int down, int keynum )
{
	if( keynum == K_F6 )
	{
		if( down )
			g_menu = !g_menu;
		return 0;
	}

	if( !g_menu )
		return 1;
	if( !down )
	{
		if( keynum == K_ESCAPE || ( keynum >= '0' && keynum <= '7' ))
			return 0;
		return 1;
	}

	if( keynum == K_ESCAPE || keynum == '0' )
	{
		g_menu = 0;
		return 0;
	}
	if( keynum == '1' )
	{
		ToggleCvar( cl_aim_assist );
		return 0;
	}
	if( keynum == '2' )
	{
		CycleCvar( cl_aim_fov, kFovSteps, (int)( sizeof( kFovSteps ) / sizeof( kFovSteps[0] )));
		return 0;
	}
	if( keynum == '3' )
	{
		CycleCvar( cl_aim_smooth, kSmoothSteps, (int)( sizeof( kSmoothSteps ) / sizeof( kSmoothSteps[0] )));
		return 0;
	}
	if( keynum == '4' )
	{
		ToggleCvar( cl_aim_attack );
		return 0;
	}
	if( keynum == '5' )
	{
		ToggleCvar( cl_aim_head );
		return 0;
	}
	if( keynum == '6' )
	{
		ToggleCvar( cl_aim_chest );
		return 0;
	}
	if( keynum == '7' )
	{
		ToggleCvar( cl_aim_body );
		return 0;
	}
	return 1;
}