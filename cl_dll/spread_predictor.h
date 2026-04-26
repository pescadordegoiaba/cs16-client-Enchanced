#pragma once

// Spread/recoil predictor HUD dot for CS16Client/Xash3D.
// Intended for offline/LAN/training use in a client build you control.

struct local_state_s;
typedef struct local_state_s local_state_t;
struct usercmd_s;
struct client_data_s;
typedef struct client_data_s client_data_t;

void SpreadDot_Capture(local_state_t *from, local_state_t *to, struct usercmd_s *cmd,
                       double time, unsigned int random_seed);
void SpreadDot_UpdateClientData(client_data_t *cdata, float time);
void SpreadDot_Draw(float flTime);
