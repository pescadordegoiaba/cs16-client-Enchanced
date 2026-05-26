#pragma once

#include <vector>

// Simple 2D Canvas drawing library for Xash3D / cs16-client
// Designed for clean, high-quality 2D overlays (outlines, UI, etc.)
// Uses engine triangle API + pfnFillRGBA for compatibility.

struct CanvasColor
{
    unsigned char r, g, b, a;
};

class Canvas
{
public:
    Canvas();
    ~Canvas();

    // Must be called once per frame (usually in HUD_Redraw or DrawTransparentTriangles)
    void BeginFrame(int screenWidth, int screenHeight);
    void EndFrame();  // restores GL state saved in BeginFrame

    // Basic drawing
    void SetColor(int r, int g, int b, int a = 255);
    void SetColor(const CanvasColor& color);

    // Screen-space line (thickness in pixels)
    void Line(float x1, float y1, float x2, float y2, float thickness = 1.0f);

    // Multiple connected lines (polyline)
    void PolyLine(const std::vector<float>& points, float thickness = 1.0f); // points as x,y,x,y...

    // Draw a simple rectangle outline
    void Rect(float x, float y, float w, float h, float thickness = 1.0f);

    // Filled rectangle
    void FilledRect(float x, float y, float w, float h);

    // Circle (approximated with lines)
    void Circle(float x, float y, float radius, int segments = 16, float thickness = 1.0f);

    // Scissor / clip rect (screen space). Useful for contained drawing regions.
    void SetScissor(int x, int y, int w, int h);
    void ResetScissor();

    // Get current screen size
    int GetWidth() const { return m_screenWidth; }
    int GetHeight() const { return m_screenHeight; }

    // === Convenience helpers for projected 2D drawing (WorldToScreen + Canvas) ===
    // Returns true if the point is on screen (not culled behind camera).
    bool WorldToScreen(const float world[3], float& screenX, float& screenY);

    // Simple projected line in world space (useful for quick debug / custom outlines).
    void Line3D(const float from[3], const float to[3], float thickness = 1.0f);

private:
    void DrawThickLine(float x1, float y1, float x2, float y2, float thickness);

    CanvasColor m_color;
    int m_screenWidth;
    int m_screenHeight;
    bool m_frameBegun;
    int m_attribStackDepth;   // safety against unbalanced Begin/End
};

// Global instance (convenient to use from anywhere)
extern Canvas gCanvas;

// To use the engine Canvas ABI, include "engine_canvas_api.h" instead of this
// header to avoid class name collision with the C struct canvas_t.
