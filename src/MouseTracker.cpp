#include "MouseTracker.hpp"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <Carbon/Carbon.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace sapf {

#ifdef _WIN32
class WindowsMouseTracker;
#elif defined(__APPLE__)
class MacMouseTracker;
#elif defined(__linux__)
class LinuxMouseTracker;
#endif

std::unique_ptr<MouseTracker> MouseTracker::create() {
#ifdef _WIN32
    return std::make_unique<WindowsMouseTracker>();
#elif defined(__APPLE__)
    return std::make_unique<MacMouseTracker>();
#elif defined(__linux__)
    return std::make_unique<LinuxMouseTracker>();
#else
    // Fall back to a null implementation on unsupported platforms
    class NullMouseTracker : public MouseTracker {
    public:
        float getMouseX() const override { return 0.5f; }
        float getMouseY() const override { return 0.5f; }
        bool getMouseButton() const override { return false; }
    };
    return std::make_unique<NullMouseTracker>();
#endif
}

} // namespace sapf
