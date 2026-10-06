// Host nativo de Dear ImGui para o client de CS 1.6.
//
// Painéis novos desenham com a API normal de Dear ImGui. O host abre o
// frame, entrega mouse/teclado e desenha por cima do HUD com OpenGL fixo,
// o mesmo contexto que o motor já deixou corrente em HUD_Redraw.
//
// Registro, em HUD_Init ou depois:
//
//   #include "imgui.h"
//   #include "imgui_host.h"
//
//   static void MeuPainel( void * )
//   {
//       if( !ImGui::Begin( "Meu painel" ) )
//       {
//           ImGui::End();
//           return;
//       }
//       ImGui::Text( "widgets normais de Dear ImGui" );
//       ImGui::End();
//   }
//
//   ImGui_RegisterPanel( "Meu painel", MeuPainel, NULL );
//
// O painel só é chamado enquanto a interface está aberta (comando `imgui`
// ou a tecla Insert). ESC ou Insert fecha e devolve o mouse ao jogo.

#ifndef IMGUI_HOST_H
#define IMGUI_HOST_H

typedef void ( *ImGuiPanelFn )( void *user );

int  ImGui_RegisterPanel( const char *title, ImGuiPanelFn fn, void *user );
void ImGui_UnregisterPanel( int id );
int  ImGui_PanelCount( void );

void ImGui_SetOpen( int open );
int  ImGui_IsOpen( void );
void ImGui_SetMenuOpen( int open );

void ImGui_Init( void );
void ImGui_Shutdown( void );
void ImGui_VidInit( void );
void ImGui_Frame( void );
// 0 = o client consumiu a tecla. 1 = o motor segue o binding.
int  ImGui_KeyEvent( int down, int keynum );

#endif
