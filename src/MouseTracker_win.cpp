#include "MouseTracker.hpp"

#ifdef _WIN32
#include <windows.h>

namespace sapf {

class WindowsMouseTracker : public MouseTracker {
public:
    WindowsMouseTracker() {
        // Get screen dimensions
        screenWidth_ = GetSystemMetrics(SM_CXSCREEN);
        screenHeight_ = GetSystemMetrics(SM_CYSCREEN);
        
        // Calculate reciprocal of screen dimensions for normalization
        rscreenWidth_ = 1.0f / screenWidth_;
        rscreenHeight_ = 1.0f / screenHeight_;
        
        // Start tracking thread
        running_ = true;
        trackingThread_ = std::thread(&WindowsMouseTracker::trackMouse, this);
    }
    
    ~WindowsMouseTracker() override {
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
            POINT p;
            if (GetCursorPos(&p)) {
                mouseX_.store(p.x * rscreenWidth_);
                // Convert to bottom-up coordinate system (0 at bottom, 1 at top)
                mouseY_.store(1.0f - p.y * rscreenHeight_);
                mouseButton_.store((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
            }
            
            // Sleep for ~16.7ms (60fps)
            Sleep(17);
        }
    }
    
    std::thread trackingThread_;
    std::atomic<bool> running_{false};
    std::atomic<float> mouseX_{0.0f};
    std::atomic<float> mouseY_{0.0f};
    std::atomic<bool> mouseButton_{false};
    int screenWidth_;
    int screenHeight_;
    float rscreenWidth_;
    float rscreenHeight_;
};

#endif // _WIN32

} // namespace sapf
