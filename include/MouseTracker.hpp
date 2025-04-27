#pragma once

#include <thread>
#include <atomic>

/**
 * Cross-platform mouse position tracking.
 * On Linux, works only with X11 or XWayland, not native Wayland.
 */
class MouseTracker {
public:
    MouseTracker();
    ~MouseTracker();

    /**
     * Get the current mouse X position normalized between 0.0 and 1.0
     */
    float getMouseX() const;

    /**
     * Get the current mouse Y position normalized between 0.0 and 1.0
     * (0 is bottom, 1 is top of screen)
     */
    float getMouseY() const;
private:
    void trackMouse();
    
#ifdef _WIN32
    const int mScreenWidth;
    const int mScreenHeight;
    const float mRscreenWidth;
    const float mRscreenHeight; 
#endif
    std::atomic<float> mMouseX{0.5f};
    std::atomic<float> mMouseY{0.5f};
    std::atomic<bool> mRunning{false};
    std::thread mTrackingThread;
    std::atomic<bool> mTrackingEnabled{false};
};