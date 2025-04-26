#pragma once

#include <thread>
#include <atomic>
#include <memory>
#include <cstdint>

namespace sapf {

/**
 * MouseTracker interface for cross-platform mouse position tracking.
 * 
 * Provides a common interface for tracking mouse position across
 * different platforms (Windows, Linux, macOS).
 */
class MouseTracker {
public:
    virtual ~MouseTracker() = default;

    /**
     * Get the current mouse X position normalized between 0.0 and 1.0
     */
    virtual float getMouseX() const = 0;

    /**
     * Get the current mouse Y position normalized between 0.0 and 1.0
     * (0 is bottom, 1 is top of screen)
     */
    virtual float getMouseY() const = 0;

    /**
     * Get the current mouse button state (true if pressed)
     */
    virtual bool getMouseButton() const = 0;

    /**
     * Create a platform-specific MouseTracker instance
     */
    static std::unique_ptr<MouseTracker> create();
};

} // namespace sapf
