#pragma once

namespace zoom {
    // Mobile UI
    constexpr float kBackButtonOpacity = 100.0f;
    constexpr float kBackButtonSizeMult = 1.15f;
    constexpr float kBackButtonScale = 0.6f;
    constexpr int kZoomLayerZOrder = 11;
    constexpr int kTouchPriority = -250;
    constexpr int kForcePriority = 2;
    constexpr float kTouchZoomDivisor = 100.0f;
    constexpr float kPauseZoomButtonScale = 0.6f;

    // Desktop
    constexpr float kSensitivityScale = 0.1f;
    constexpr float kSmoothScrollDamping = 0.1f;
    constexpr float kAutoShowZoomThreshold = 1.01f;

    // Shared
    constexpr float kMinZoom = 1.0f;
    constexpr float kDefaultZoom = 1.0f;
}
