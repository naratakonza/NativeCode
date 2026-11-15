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
  static const _labels = [
    'TAB',
    'ALT',
    'SPC',
    'SHIFT',
    'F',
    'C',
    'H',
    'Y',
    'N',
    '2'
  ];
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
        final gap = 6 * scale;
        final padding = 7 * scale + 1; // Includes the panel's 1 dp border.
        final left = math.max(safe.left + 8, width * anchor.dx);
        final top = math.max(safe.top, height * anchor.dy) + 13 * scale;
        final availableWidth = width - safe.right - 8 - left - 2 * padding;
        final columns = math.min(_labels.length,
            ((availableWidth + gap) / (capWidth + gap)).floor());
        if (columns < 1) return const SizedBox.shrink();

        final rows = (_labels.length / columns).ceil();
        final barHeight = rows * capHeight + (rows - 1) * gap + 2 * padding;
        if (top + barHeight > height - safe.bottom - 8) {
          return const SizedBox.shrink();
        }

        return Stack(
          children: [
            Positioned(
              left: left,
              top: top,
              width: columns * (capWidth + gap) - gap + 2 * padding + 0.01,
              child: IgnorePointer(
                child: RepaintBoundary(
                  child: Container(
                    key: const ValueKey('samp_keys_bar'),
                    padding: EdgeInsets.all(padding - 1),
                    decoration: BoxDecoration(
                      borderRadius: BorderRadius.circular(13 * scale),
                      border: Border.all(color: const Color(0x60CFE7FF)),
                      gradient: const LinearGradient(
                        begin: Alignment.topLeft,
                        end: Alignment.bottomRight,
                        colors: [Color(0x94324053), Color(0xA9141823)],
                      ),
                      boxShadow: const [
                        BoxShadow(
                          color: Color(0x33000000),
                          offset: Offset(0, 3),
                          blurRadius: 8,
                        ),
                      ],
                    ),
                    child: Wrap(
                      spacing: gap,
                      runSpacing: gap,
                      children: [
                        for (final label in _labels)
                          _KeyCap(
                            label: label,
                            width: capWidth,
                            height: capHeight,
                            scale: scale,
                          ),
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

class _KeyCap extends StatelessWidget {
  const _KeyCap({
    required this.label,
    required this.width,
    required this.height,
    required this.scale,
  });

  final String label;
  final double width;
  final double height;
  final double scale;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: width,
      height: height,
      alignment: Alignment.center,
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(7 * scale),
        border: Border.all(color: const Color(0x5EE7F1FF)),
        gradient: const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [Color(0x46F0F7FF), Color(0x146680A5)],
        ),
      ),
      child: Text(
        label,
        maxLines: 1,
        style: TextStyle(
          fontFamily: 'Rajdhani',
          fontSize: 13 * scale,
          fontWeight: FontWeight.w600,
          letterSpacing: 0.25,
          color: const Color(0xFFF3F8FF),
          decoration: TextDecoration.none,
          shadows: const [Shadow(color: Color(0x99000000), blurRadius: 2)],
        ),
      ),
    );
  }
}
