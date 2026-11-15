import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class SampKeysBar extends StatefulWidget {
  const SampKeysBar({super.key});

  static const channel = MethodChannel('flutter_keybar_preview');

  @override
  State<SampKeysBar> createState() => _SampKeysBarState();
}

class _SampKeysBarState extends State<SampKeysBar> {
  Offset? _radarAnchor;

  @override
  void initState() {
    super.initState();
    SampKeysBar.channel.setMethodCallHandler(_handlePreview);
  }

  @override
  void dispose() {
    SampKeysBar.channel.setMethodCallHandler(null);
    super.dispose();
  }

  Future<void> _handlePreview(MethodCall call) async {
    if (!mounted) return;

    Offset? anchor;
    if (call.method == 'show') {
      final args = call.arguments;
      if (args is! Map) return;
      final left = args['radarLeft'];
      final bottom = args['radarBottom'];
      if (left is! int ||
          left < 0 ||
          left >= 1000 ||
          bottom is! int ||
          bottom <= 0 ||
          bottom >= 1000) {
        return;
      }
      anchor = Offset(left / 1000, bottom / 1000);
    } else if (call.method != 'hide') {
      return;
    }

    if (_radarAnchor != anchor) setState(() => _radarAnchor = anchor);
  }

  @override
  Widget build(BuildContext context) {
    final anchor = _radarAnchor;
    if (anchor == null || MediaQuery.viewInsetsOf(context).bottom > 0) {
      return const SizedBox.shrink();
    }

    final safe = MediaQuery.viewPaddingOf(context);
    return LayoutBuilder(
      builder: (context, constraints) {
        final width = constraints.maxWidth;
        final height = constraints.maxHeight;
        final scale = (width / 850).clamp(0.85, 1.15).toDouble();
        final capWidth = 39 * scale;
        final capHeight = 27 * scale;
        // Keep the first button in its previous position below the radar.
        final inset = 7 * scale + 1;
        final left = math.max(safe.left + 8, width * anchor.dx) + inset;
        final top = math.max(safe.top, height * anchor.dy) + 13 * scale + inset;
        if (left + capWidth > width - safe.right - 8 ||
            top + capHeight > height - safe.bottom - 8) {
          return const SizedBox.shrink();
        }

        return Stack(
          children: [
            Positioned(
              left: left,
              top: top,
              child: IgnorePointer(
                child: Container(
                  key: const ValueKey('samp_keys_bar'),
                  width: capWidth,
                  height: capHeight,
                  alignment: Alignment.center,
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(7 * scale),
                    border: Border.all(color: const Color(0x80E7F1FF)),
                  ),
                  child: Text(
                    '…',
                    style: TextStyle(
                      fontSize: 18 * scale,
                      fontWeight: FontWeight.w700,
                      color: const Color(0xFFF3F8FF),
                      decoration: TextDecoration.none,
                      shadows: const [
                        Shadow(color: Color(0xB3000000), blurRadius: 2),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ],
        );
      },
    );
  }
}
