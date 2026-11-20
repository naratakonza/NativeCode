import 'dart:math' as math;
import 'package:flutter/widgets.dart';

class SampOverlayLayout {
  static bool isTablet(Size size) => size.shortestSide >= 600.0;

  static double tabletPanelBottom(Size size, EdgeInsets safe) {
    final scale = math.min(
      (size.width - safe.horizontal) / 970.0,
      (size.height - safe.vertical) / 540.0,
    );
    final gap = math.max(10.0, 12.0 * scale);
    // แนว FPS ใน HUD จากภาพอยู่ประมาณ 45% ของความสูงจอ.
    return (size.height * 0.45 - gap)
        .clamp(safe.top, math.max(safe.top, size.height - safe.bottom));
  }
}
