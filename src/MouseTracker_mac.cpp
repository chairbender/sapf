#include "MouseTracker.hpp"

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <unistd.h>

namespace sapf {

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

} // namespace sapf
