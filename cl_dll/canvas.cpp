#include "canvas.h"
#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifdef fabs
#undef fabs
#endif

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include <cmath>

Canvas gCanvas;

Canvas::Canvas()
{
    m_color = {255, 255, 255, 255};
    m_screenWidth = 640;
    m_screenHeight = 480;
    m_frameBegun = false;
    m_attribStackDepth = 0;
}

Canvas::~Canvas()
{
}

void Canvas::BeginFrame(int screenWidth, int screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_frameBegun = true;
    m_attribStackDepth++;

    // Save GL state to avoid corrupting HUD / other 2D draws (robustness improvement)
    glPushAttrib( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TRANSFORM_BIT | GL_SCISSOR_BIT );

    // 2D overlay friendly state
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

void Canvas::EndFrame()
{
    if (m_frameBegun && m_attribStackDepth > 0)
    {
        glPopAttrib();
        m_attribStackDepth--;
        if (m_attribStackDepth == 0)
            m_frameBegun = false;
    }
}

void Canvas::SetColor(int r, int g, int b, int a)
{
    m_color.r = (unsigned char)((r < 0) ? 0 : (r > 255 ? 255 : r));
    m_color.g = (unsigned char)((g < 0) ? 0 : (g > 255 ? 255 : g));
    m_color.b = (unsigned char)((b < 0) ? 0 : (b > 255 ? 255 : b));
    m_color.a = (unsigned char)((a < 0) ? 0 : (a > 255 ? 255 : a));
}

void Canvas::SetColor(const CanvasColor& color)
{
    m_color = color;
}

// Internal: draws a thick line using two triangles (quad)
void Canvas::DrawThickLine(float x1, float y1, float x2, float y2, float thickness)
{
    if (thickness <= 0.0f) thickness = 1.0f;

    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) return;

    float nx = -dy / len;
    float ny =  dx / len;

    float half = thickness * 0.5f;

    float x1a = x1 + nx * half;
    float y1a = y1 + ny * half;
    float x1b = x1 - nx * half;
    float y1b = y1 - ny * half;

    float x2a = x2 + nx * half;
    float y2a = y2 + ny * half;
    float x2b = x2 - nx * half;
    float y2b = y2 - ny * half;

    // pTriAPI draw - frame Begin/End manages overall GL state. Set trans for this primitive.
    gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
    gEngfuncs.pTriAPI->Begin(TRI_QUADS);

    gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, m_color.a);

    gEngfuncs.pTriAPI->Vertex3f(x1a, y1a, 0);
    gEngfuncs.pTriAPI->Vertex3f(x2a, y2a, 0);
    gEngfuncs.pTriAPI->Vertex3f(x2b, y2b, 0);
    gEngfuncs.pTriAPI->Vertex3f(x1b, y1b, 0);

    gEngfuncs.pTriAPI->End();
    // Do NOT force kRenderNormal here - allows batching multiple canvas draws in one frame.
    // EndFrame() + glPopAttrib will restore the GL state the caller had before BeginFrame.
}

void Canvas::Line(float x1, float y1, float x2, float y2, float thickness)
{
    if (!m_frameBegun) return;
    DrawThickLine(x1, y1, x2, y2, thickness);
}

void Canvas::PolyLine(const std::vector<float>& points, float thickness)
{
    if (!m_frameBegun || points.size() < 4) return; // need at least 2 points (4 floats)

    // Draw connected line segments (proper polyline)
    for (size_t i = 0; i + 3 < points.size(); i += 2)
    {
        DrawThickLine(points[i], points[i+1], points[i+2], points[i+3], thickness);
    }
}

void Canvas::Rect(float x, float y, float w, float h, float thickness)
{
    if (!m_frameBegun) return;

    std::vector<float> pts = {
        x, y,
        x + w, y,
        x + w, y + h,
        x, y + h,
        x, y   // close
    };
    PolyLine(pts, thickness);
}

void Canvas::FilledRect(float x, float y, float w, float h)
{
    if (!m_frameBegun) return;

    gEngfuncs.pfnFillRGBA((int)x, (int)y, (int)w, (int)h, m_color.r, m_color.g, m_color.b, m_color.a);
}

void Canvas::Circle(float x, float y, float radius, int segments, float thickness)
{
    if (!m_frameBegun || segments < 3) return;

    std::vector<float> pts;
    pts.reserve(segments * 2 + 2);

    for (int i = 0; i <= segments; ++i)
    {
        float ang = (float)i / segments * 2.0f * 3.14159265f;
        pts.push_back(x + cosf(ang) * radius);
        pts.push_back(y + sinf(ang) * radius);
    }

    PolyLine(pts, thickness);
}

void Canvas::SetScissor(int x, int y, int w, int h)
{
	if (!m_frameBegun) return;
	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, w, h);
}

void Canvas::ResetScissor()
{
	if (!m_frameBegun) return;
	glDisable(GL_SCISSOR_TEST);
}

// WorldToScreen wrapper (returns true if visible on screen)
bool Canvas::WorldToScreen(const float world[3], float& screenX, float& screenY)
{
	if (!gEngfuncs.pTriAPI || m_screenWidth <= 0 || m_screenHeight <= 0)
		return false;

	float screen[2];
	if (gEngfuncs.pTriAPI->WorldToScreen(const_cast<float*>(world), screen) == 0)
	{
		// Convert from [-1,1] normalized device coords to screen pixels
		screenX = (screen[0] + 1.0f) * 0.5f * m_screenWidth;
		screenY = (1.0f - screen[1]) * 0.5f * m_screenHeight; // Y is flipped in NDC
		return true;
	}
	return false;
}

// Simple 3D line projected to 2D using the Canvas
void Canvas::Line3D(const float from[3], const float to[3], float thickness)
{
	if (!m_frameBegun || !gEngfuncs.pTriAPI) return;

	float sx1, sy1, sx2, sy2;
	if (WorldToScreen(from, sx1, sy1) && WorldToScreen(to, sx2, sy2))
	{
		Line(sx1, sy1, sx2, sy2, thickness);
	}
}
