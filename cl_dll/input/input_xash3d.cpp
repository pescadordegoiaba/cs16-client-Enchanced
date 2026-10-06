//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Vazio. A implementacao de input vive em input_sdl.cpp.
//
// $NoKeywords: $
//=============================================================================

// Nada a definir aqui. O backend de input (mouse, botoes, joystick, cvars
// relacionadas, callbacks IN_*) esta implementado em input_sdl.cpp.
//
// Se um dia o backend mudar (por exemplo input_evdev.cpp), remova o
// input_sdl.cpp do CMakeLists e coloque o novo arquivo aqui — nunca os
// dois ao mesmo tempo, senao os simbolos colidem no linker.