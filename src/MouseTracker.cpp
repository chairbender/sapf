#include "MouseTracker.hpp"

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
#include <libinput.h>
#include <libudev.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>

MouseTracker::MouseTracker() 
    : mMouseX(0.5f),  // Start at center of screen
      mMouseY(0.5f),
      mRunning(true)
{
    // Start the tracking thread
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

static int open_restricted(const char *path, int flags, void *user_data) {
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
    close(fd);
}

void MouseTracker::trackMouse() {
    // Initialize libudev context
    struct udev *udev = udev_new();
    if (!udev) {
        return;
    }

    // Setup libinput interface
    static const struct libinput_interface interface = {
        .open_restricted = open_restricted,
        .close_restricted = close_restricted,
    };

    // Create libinput context
    struct libinput *li = libinput_udev_create_context(&interface, nullptr, udev);
    if (!li) {
        udev_unref(udev);
        return;
    }

    // Assign seat to libinput context
    if (libinput_udev_assign_seat(li, "seat0") != 0) {
        libinput_unref(li);
        udev_unref(udev);
        return;
    }

    // Get the initial screen dimensions for normalization
    // Initialize with reasonable defaults
    float screenWidth = 1920.0f;  // Default screen width
    float screenHeight = 1080.0f; // Default screen height
    
    // Attempt to get actual screen dimensions
    // TODO: attempt to do this for X11 but it won't be possible on
    //  wayland unless we create a window (or can we at least access it over the parent terminal?)
    
    // For normalization
    float rscreenWidth = 1.0f / screenWidth;
    float rscreenHeight = 1.0f / screenHeight;

    // Track absolute mouse position (we'll convert to normalized coordinates)
    float absoluteX = screenWidth / 2.0f;
    float absoluteY = screenHeight / 2.0f;
    
    // Setup polling
    struct pollfd fds;
    fds.fd = libinput_get_fd(li);
    fds.events = POLLIN;
    fds.revents = 0;
    
    // Main event loop
    while (mRunning) {
        // Poll for events with a timeout
        int poll_result = poll(&fds, 1, 17); // ~60fps

        if (poll_result > 0) {
            libinput_dispatch(li);
            
            struct libinput_event *event;
            while ((event = libinput_get_event(li)) != nullptr) {
                enum libinput_event_type event_type = libinput_event_get_type(event);
                
                if (event_type == LIBINPUT_EVENT_POINTER_MOTION) {
                    // Handle relative mouse motion
                    struct libinput_event_pointer *pointer_event = 
                        libinput_event_get_pointer_event(event);
                    
                    double dx = libinput_event_pointer_get_dx(pointer_event);
                    double dy = libinput_event_pointer_get_dy(pointer_event);
                    
                    // Update absolute position with relative motion
                    absoluteX += dx;
                    absoluteY += dy;
                    
                    // Clamp to screen boundaries
                    absoluteX = std::max(0.0f, std::min(screenWidth, absoluteX));
                    absoluteY = std::max(0.0f, std::min(screenHeight, absoluteY));
                    
                    // Convert to normalized coordinates (0.0 - 1.0)
                    mMouseX.store(absoluteX * rscreenWidth);
                    // Flip Y to have 0 at bottom, 1 at top
                    mMouseY.store(1.0f - (absoluteY * rscreenHeight));
                }
                else if (event_type == LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE) {
                    // Handle absolute mouse motion (e.g., from touchscreens)
                    struct libinput_event_pointer *pointer_event = 
                        libinput_event_get_pointer_event(event);
                    
                    // Get normalized coordinates from libinput (0.0 - 1.0)
                    double x = libinput_event_pointer_get_absolute_x_transformed(
                        pointer_event, screenWidth);
                    double y = libinput_event_pointer_get_absolute_y_transformed(
                        pointer_event, screenHeight);
                    
                    // Update absolute position
                    absoluteX = x;
                    absoluteY = y;
                    
                    // Store normalized coordinates
                    mMouseX.store(x * rscreenWidth);
                    // Flip Y to have 0 at bottom, 1 at top
                    mMouseY.store(1.0f - (y * rscreenHeight));
                }
                
                libinput_event_destroy(event);
            }
        }
        
        if (poll_result == 0) {
            // Timeout - no events
            usleep(1000); // Small sleep to avoid busy waiting
        }
    }
    
    // Cleanup
    libinput_unref(li);
    udev_unref(udev);
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