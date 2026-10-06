#ifndef IMGUI_MENU_H
#define IMGUI_MENU_H

void ImGuiMenu_Init( void );
void ImGuiMenu_Open( int menuType, int bits );
void ImGuiMenu_Close( void );
int  ImGuiMenu_IsOpen( void );
// 1 = tecla consumida
int  ImGuiMenu_OnKey( int down, int keynum );

#endif
