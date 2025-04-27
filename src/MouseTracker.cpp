#include "MouseTracker.hpp"

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <unistd.h>

class MacMouseTracker : public MouseTracker {
public:
    MacMouseTracker() {
        // Get main display information
        CGDirectDisplayID display = kCGDirectMainDisplay;
        CGRect bounds = CGDisplayBounds(display);
        
        // Calculate reciprocal of screen dimensions for normalization
        rscreenWidth_ = 1.0f / bounds.size.width;
        rscreenHeight_ = 1.0f / bounds.size.height;
        
        // Start tracking thread
        running_ = true;
        trackingThread_ = std::thread(&MacMouseTracker::trackMouse, this);
    }
    
    ~MacMouseTracker() override {
        running_ = false;
        if (trackingThread_.joinable()) {
            trackingThread_.join();
        }
    }
    
    float getMouseX() const override {
        return mouseX_.load();
    }
    
    float getMouseY() const override {
        return mouseY_.load();
    }
    
    bool getMouseButton() const override {
        return mouseButton_.load();
    }
    
private:
    void trackMouse() {
        while (running_) {
            HIPoint point;
            HICoordinateSpace space = kHICoordSpaceScreenPixel;
            HIGetMousePosition(space, nullptr, &point);
            
            mouseX_.store(point.x * rscreenWidth_);
            // Convert to bottom-up coordinate system (0 at bottom, 1 at top)
            mouseY_.store(1.0f - point.y * rscreenHeight_);
            mouseButton_.store(GetCurrentButtonState());
            
            usleep(17000); // ~60fps update rate
        }
    }
    
    // Helper to check mouse button state
    bool GetCurrentButtonState() {
        return CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft);
    }
    
    std::thread trackingThread_;
    std::atomic<bool> running_{false};
    std::atomic<float> mouseX_{0.0f};
    std::atomic<float> mouseY_{0.0f};
    std::atomic<bool> mouseButton_{false};
    float rscreenWidth_;
    float rscreenHeight_;
};

#endif // __APPLE__

#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <thread>
#include <cstring>

class LinuxMouseTracker : public MouseTracker {
public:
    LinuxMouseTracker() 
        : mouseX_(0.5f)  // Start at the center of the screen
        , mouseY_(0.5f)  // Start at the center of the screen
        , mouseButton_(false)
        , sensitivity_(0.001f) // Adjust sensitivity as needed
    {
        // Find a mouse input device
        findMouseDevice();
        
        // Start tracking thread
        running_ = true;
        trackingThread_ = std::thread(&LinuxMouseTracker::trackMouse, this);
    }
    
    ~LinuxMouseTracker() override {
        running_ = false;
        if (trackingThread_.joinable()) {
            trackingThread_.join();
        }
        
        if (mouseFd_ >= 0) {
            close(mouseFd_);
        }
    }
    
    float getMouseX() const override {
        return mouseX_.load();
    }
    
    float getMouseY() const override {
        return mouseY_.load();
    }
    
    bool getMouseButton() const override {
        return mouseButton_.load();
    }
    
private:
    void findMouseDevice() {
        // Try to open common mouse input devices
        const char* devices[] = {
            "/dev/input/event0",
            "/dev/input/event1",
            "/dev/input/event2",
            "/dev/input/event3",
            "/dev/input/event4",
            "/dev/input/mice",
            "/dev/input/mouse0"
        };
        
        for (const char* device : devices) {
            mouseFd_ = open(device, O_RDONLY | O_NONBLOCK);
            if (mouseFd_ >= 0) {
                // We found a device that we can open
                char name[256];
                if (ioctl(mouseFd_, EVIOCGNAME(sizeof(name)), name) >= 0) {
                    // We could get the name, check if it's a mouse
                    if (strstr(name, "mouse") || strstr(name, "Mouse") || strstr(name, "touchpad") || strstr(name, "Touchpad")) {
                        return;
                    }
                }
                // Not a mouse, try next device
                close(mouseFd_);
                mouseFd_ = -1;
            }
        }
    }
    
    void trackMouse() {
        struct input_event event;
        
        while (running_) {
            if (mouseFd_ >= 0) {
                // Try to read from the mouse device
                while (read(mouseFd_, &event, sizeof(event)) > 0) {
                    if (event.type == EV_REL) {
                        if (event.code == REL_X) {
                            // Update X position (relative movement)
                            float x = mouseX_.load() + (event.value * sensitivity_);
                            // Clamp between 0 and 1 for safety
                            // Note: For Wayland/X11, we're tracking relative motion only
                            mouseX_.store(std::max(0.0f, std::min(1.0f, x)));
                        }
                        else if (event.code == REL_Y) {
                            // Update Y position (relative movement)
                            // Note: Linux input Y increases downward, so negate for consistency
                            float y = mouseY_.load() - (event.value * sensitivity_);
                            // Clamp between 0 and 1 for safety
                            mouseY_.store(std::max(0.0f, std::min(1.0f, y)));
                        }
                    }
                    else if (event.type == EV_KEY && event.code == BTN_LEFT) {
                        // Update button state
                        mouseButton_.store(event.value == 1);
                    }
                }
            }
            
            // Sleep for ~16.7ms (60fps)
            usleep(17000);
        }
    }
    
    std::thread trackingThread_;
    std::atomic<bool> running_{false};
    std::atomic<float> mouseX_{0.0f};
    std::atomic<float> mouseY_{0.0f};
    std::atomic<bool> mouseButton_{false};
    float sensitivity_;
    int mouseFd_{-1};
};

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

float MouseTracker::getMouseX() const {
    return mMouseX.load();
}

float MouseTracker::getMouseY() const {
    return mMouseY.load();
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