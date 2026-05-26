/***
 *
 *  engine_canvas_api.h
 *  Shared Canvas ABI declaration.
 *  Avoids name collision with the local C++ class "Canvas" defined in "canvas.h".
 *
 *  Recommended usage:
 *     #include "engine_canvas_api.h"
 *     ...
 *     canvas_t *c = ...; // supplied by the engine/renderer integration point
 *     if (c && c->BeginFrame) {
 *         c->BeginFrame(width, height);
 *         // draw...
 *         c->EndFrame();
 *     }
 *
 ****/

#ifndef ENGINE_CANVAS_API_H
#define ENGINE_CANVAS_API_H

#define CANVAS_API_VERSION 1

// Try to include the real engine header from common locations used in this project.
// Falls back to a minimal compatible declaration so code using the pointer still compiles.
#if defined(__has_include)
#  if __has_include("../../xash3d-enhanced/common/canvas.h")
#    include "../../xash3d-enhanced/common/canvas.h"
#  elif __has_include("../common/canvas.h")
#    include "../common/canvas.h"
#  elif __has_include("common/canvas.h")
#    include "common/canvas.h"
#  else
     // Self-contained minimal declaration (must match the one in xash3d-enhanced/common/canvas.h)
     typedef struct canvas_s
     {
         int version;
         void (*BeginFrame)( int width, int height );
         void (*EndFrame)( void );
         void (*SetColor)( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
         void (*SetColorf)( float r, float g, float b, float a );
         void (*Line)( float x1, float y1, float x2, float y2, float thickness );
         void (*PolyLine)( const float *points, int numPoints, float thickness );
         void (*Rect)( float x, float y, float w, float h, float thickness );
         void (*FilledRect)( float x, float y, float w, float h );
         void (*Circle)( float x, float y, float radius, int segments, float thickness );
         void (*SetScissor)( int x, int y, int w, int h );
         void (*ResetScissor)( void );
     } canvas_t;
#  endif
#else
   // Older compilers: try common relative path
#  include "../../xash3d-enhanced/common/canvas.h"
#endif

#endif // ENGINE_CANVAS_API_H
