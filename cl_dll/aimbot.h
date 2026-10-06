#pragma once

/* Aim assist global (sem trava offline). */

struct aim_box_t
{
	int player;
	int group;
	float origin[3];
};

void Aimbot_Init( void );
void Aimbot_SetBoxes( const aim_box_t *boxes, int count );
int Aimbot_Apply( float viewangles[3], int buttons );
void Aimbot_Draw( void );
/* 0 = key consumed, 1 = pass to the engine. */
int Aimbot_Key( int down, int keynum );