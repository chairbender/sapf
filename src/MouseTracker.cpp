#include "MouseTracker.hpp"


float MouseTracker::getMouseX() const {
    return mMouseX.load();
}

float MouseTracker::getMouseY() const {
    return mMouseY.load();
}

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <unistd.h>

MouseTracker::MouseTracker() {
    // Get main display information
    CGDirectDisplayID display = kCGDirectMainDisplay;
    CGRect bounds = CGDisplayBounds(display);
    
    // Calculate reciprocal of screen dimensions for normalization
    float rscreenWidth = 1.0f / bounds.size.width;
    float rscreenHeight = 1.0f / bounds.size.height;
    
    // Start tracking thread
    mRunning = true;
    mTrackingThread = std::thread(&MouseTracker::trackMouse, this);
}

MouseTracker::~MouseTracker() {
    mRunning = false;
    if (mTrackingThread.joinable()) {
        mTrackingThread.join();
    }
}

float MouseTracker::getMouseX() const {
    return mMouseX.load();
}

float MouseTracker::getMouseY() const {
    return mMouseY.load();
}

void MouseTracker::trackMouse() {
    // Get main display information
    CGDirectDisplayID display = kCGDirectMainDisplay;
    CGRect bounds = CGDisplayBounds(display);
    
    // Calculate reciprocal of screen dimensions for normalization
    float rscreenWidth = 1.0f / bounds.size.width;
    float rscreenHeight = 1.0f / bounds.size.height;
    
    while (mRunning) {
        HIPoint point;
        HICoordinateSpace space = kHICoordSpaceScreenPixel;
        HIGetMousePosition(space, nullptr, &point);
        
        mMouseX.store(point.x * rscreenWidth);
        // Convert to bottom-up coordinate system (0 at bottom, 1 at top)
        mMouseY.store(1.0f - point.y * rscreenHeight);
        
        usleep(17000); // ~60fps update rate
    }
}

#endif // __APPLE__

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <thread>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

MouseTracker::MouseTracker()
    : mRunning{true},
    mTrackingThread(std::thread(&MouseTracker::trackMouse, this))
{}

MouseTracker::~MouseTracker() {
    mRunning = false;
    if (mTrackingThread.joinable()) {
        mTrackingThread.join();
    }
}

void MouseTracker::trackMouse() {
    // Try to open X display (should work in X11 or XWayland)
    const auto display{XOpenDisplay(NULL)};
    if (!display) {
        printf("MouseTracker: Could not open X display, mouse tracking disabled\n");
        return;
    }
    
    // Get screen information
    const auto screen{DefaultScreen(display)};
    const auto root{RootWindow(display, screen)};
    const auto width{DisplayWidth(display, screen)};
    const auto height{DisplayHeight(display, screen)};
    
    // Calculate reciprocal screen dimensions for normalization
    const auto rwidth{1.0f / static_cast<float>(width)};
    const auto rheight{1.0f / static_cast<float>(height)};
    
    // Main tracking loop
    while (mRunning) {
        Window root_return, child_return;
        int root_x, root_y, win_x, win_y;
        unsigned int mask_return;
        
        const auto result{XQueryPointer(
            display,
            root,
            &root_return,
            &child_return,
            &root_x, &root_y,
            &win_x, &win_y,
            &mask_return
        )};
        
        if (result) {
            mMouseX.store(static_cast<float>(root_x) * rwidth);
            // Convert to bottom-up coordinate system (0 at bottom, 1 at top)
            mMouseY.store(1.0f - static_cast<float>(root_y) * rheight);
        }
        
        // Sleep to prevent high CPU usage
        usleep(17000); // ~60Hz update rate
    }
    
    // Cleanup
    XCloseDisplay(display);
}
#endif // __linux__

#ifdef _WIN32
#include <windows.h>
#include <thread>

MouseTracker::MouseTracker() :
    mScreenWidth{GetSystemMetrics(SM_CXSCREEN)},
    mScreenHeight{GetSystemMetrics(SM_CYSCREEN)},
    mRscreenWidth{1.0f / mScreenWidth},
    mRscreenHeight{1.0f / mScreenHeight},
    mRunning{true},
    mTrackingThread{std::thread(&MouseTracker::trackMouse, this)}
    {
}

MouseTracker::~MouseTracker() {
    mRunning = false;
    if (mTrackingThread.joinable()) {
        mTrackingThread.join();
    }
}

void MouseTracker::trackMouse() {
    while (mRunning) {
        POINT p;
        if (GetCursorPos(&p)) {
            mMouseX.store(p.x * mRscreenWidth);
            // Convert to bottom-up coordinate system (0 at bottom, 1 at top)
            mMouseY.store(1.0f - p.y * mRscreenHeight);
        }
        
        // Sleep for ~16.7ms (60fps)
        Sleep(17);
    }
}
#endif // _WIN32