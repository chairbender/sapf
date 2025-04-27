#pragma once

#ifdef _WIN32
#include <thread>
#endif

/**
 * TODO: Convert to non-virtual
 * Cross-platform mouse position tracking.
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

#ifdef _WIN32
    void trackMouse();
    
    const int mScreenWidth;
    const int mScreenHeight;
    const float mRscreenWidth;
    const float mRscreenHeight; 
    std::atomic<bool> mRunning{false};
    std::thread mTrackingThread;
    std::atomic<float> mMouseX{0.0f};
    std::atomic<float> mMouseY{0.0f};
#endif
};