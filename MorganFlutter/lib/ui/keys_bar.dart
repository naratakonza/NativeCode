import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';
import 'dart:math' as math;

import 'samp_chat.dart';

class KeysBarBridge {
  static const channel = MethodChannel('flutter_keysbar');
  static const _uiChannel = MethodChannel('flutter/ui_control');
  static final state = ValueNotifier<Map<dynamic, dynamic>>({});

  static void init() {
    _uiChannel.setMethodCallHandler((call) async {
      if (call.method == 'UpdateKeysBar' && call.arguments is Map) {
        state.value = Map<dynamic, dynamic>.from(call.arguments as Map);
      }
    });
    WidgetsBinding.instance.addPostFrameCallback((_) {
      unawaited(invoke('ready'));
    });
  }

  static Future<void> invoke(String method, [Object? args]) async {
    try {
      await channel.invokeMethod<bool>(method, args);
    } on MissingPluginException {
      // Standalone Flutter preview has no native game bridge.
    } on PlatformException catch (error) {
      debugPrint('flutter_keysbar $method: ${error.code}');
    }
  }

  static void reportHitRegion(Rect rect) {
    unawaited(invoke('hitRegion', {
      'x': rect.left,
      'y': rect.top,
      'width': rect.width,
      'height': rect.height,
    }));
  }
}

class SampKeysBarOverlay extends StatelessWidget {
  const SampKeysBarOverlay({super.key});

  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder<Map<dynamic, dynamic>>(
      valueListenable: KeysBarBridge.state,
      builder: (context, data, _) => ValueListenableBuilder<bool>(
        valueListenable: SampChatController.inputActive,
        builder: (context, inputActive, _) => Offstage(
          // Hide immediately, retaining the expanded/collapsed state.
          offstage: inputActive,
          child: SampKeysBar(
            visible: data['visible'] == true && !inputActive,
            isInVehicle: data['isInVehicle'] == true,
            isMenuActive: data['isMenuActive'] == true,
            isF4Enabled: data['isF4Enabled'] == true,
            radarLeft: (data['radarLeft'] as num?)?.toDouble() ?? -1,
            radarBottom: (data['radarBottom'] as num?)?.toDouble() ?? -1,
          ),
        ),
      ),
    );
  }
}

class SampKeysBar extends StatefulWidget {
  final bool visible;
  final bool isInVehicle;
  final bool isMenuActive;
  final bool isF4Enabled;
  final double radarLeft;
  final double radarBottom;

  const SampKeysBar({
    Key? key,
    required this.visible,
    required this.isInVehicle,
    required this.isMenuActive,
    required this.isF4Enabled,
    required this.radarLeft,
    required this.radarBottom,
  }) : super(key: key);

  @override
  State<SampKeysBar> createState() => _SampKeysBarState();
}

class _SampKeysBarState extends State<SampKeysBar>
    with TickerProviderStateMixin {
  final GlobalKey _panelKey = GlobalKey();
  Rect? _lastHitRegion;

  bool _isExpanded = false;
  late AnimationController _fadeController;
  late Animation<double> _fadeAnimation;

  late AnimationController _expandController;
  late Animation<double> _expandAnimation;

  @override
  void initState() {
    super.initState();
    _fadeController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 300),
    );
    _fadeAnimation = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _fadeController, curve: Curves.easeIn),
    );

    _expandController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 250),
    );
    _expandAnimation = CurvedAnimation(
      parent: _expandController,
      curve: Curves.easeInOutCubic,
    );

    _fadeController.addStatusListener((status) {
      if (status == AnimationStatus.dismissed && mounted) setState(() {});
    });
    _expandController.addListener(_reportHitRegion);
    if (widget.visible) {
      _fadeController.forward();
    }
  }

  @override
  void didUpdateWidget(covariant SampKeysBar oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.visible != oldWidget.visible) {
      if (widget.visible) {
        _fadeController.forward();
      } else {
        _fadeController.reverse();
        _lastHitRegion = null;
        KeysBarBridge.reportHitRegion(Rect.zero);
      }
    }
  }

  @override
  void dispose() {
    _fadeController.dispose();
    _expandController.dispose();
    super.dispose();
  }

  void _sendKeyPress(int keyId) {
    unawaited(KeysBarBridge.invoke('sendKeyInput', {'keyId': keyId}));
  }

  void _reportHitRegion() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted || !widget.visible) return;
      final panel = _panelKey.currentContext?.findRenderObject();
      final rect = panel is RenderBox && panel.hasSize
          ? panel.localToGlobal(Offset.zero) & panel.size
          : Rect.zero;
      if (rect != _lastHitRegion) {
        _lastHitRegion = rect;
        KeysBarBridge.reportHitRegion(rect);
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    if (!widget.visible && _fadeController.isDismissed) {
      return const SizedBox.shrink();
    }

    final media = MediaQuery.of(context);
    final screenWidth = media.size.width;
    final screenHeight = media.size.height;
    final safe = media.viewPadding;
    final usableWidth = screenWidth - safe.horizontal;
    final usableHeight = screenHeight - safe.vertical;
    final double scale = math.min(usableWidth / 970.0, usableHeight / 540.0);
    final double btnWidth = 57.5 * scale;
    final double btnHeight = 35.0 * scale;
    final double spacing = 6.0 * scale;
    final double borderRadius = 4.0 * scale;
    final double fontSize = 11.25 * scale;
    final double gap = math.max(10.0, 12.0 * scale);
    final double panelHeight = btnHeight + 6.0 * scale;
    final double maxSafeTop = screenHeight - safe.bottom - panelHeight - gap;
    final double topPos = math.max(
        safe.top + gap, widget.radarBottom * screenHeight / 1000.0 + gap);
    final double maxLeft = screenWidth - safe.right - btnWidth - 2.0 * spacing;
    final double leftPos = (widget.radarLeft * screenWidth / 1000.0)
        .clamp(safe.left, math.max(safe.left, maxLeft));
    // Wait for the actual radar bounds; never move the panel upwards over it.
    if (scale <= 0 ||
        widget.radarLeft < 0 ||
        widget.radarBottom < 0 ||
        topPos > maxSafeTop) {
      _reportHitRegion();
      return const SizedBox.shrink();
    }
    final double availableBarWidth = math.max(
        0.0, screenWidth - safe.right - leftPos - btnWidth - 3.0 * spacing);
    _reportHitRegion();

    final String altLabel = "ALT";
    final String spcLabel = "SPC";
    final String shiftLabel = widget.isInVehicle ? "LMB" : "SHIFT";
    final String enterLabel = widget.isMenuActive ? "ENTER" : "F";
    final String cLabel = "C";
    final String hLabel = "H";
    final String yLabel = "Y";
    final String nLabel = "N";
    final String tabLabel = "TAB";
    final String submissionLabel = "2";
    final String f4Label = "F4";
    final String f8Label = "F8";
    final String f11Label = "F11";
    final String upLabel = "UP";
    final String downLabel = "DOWN";
    final String leftLabel = "LEFT";
    final String rightLabel = "RIGHT";

    final bool isUpDownEnabled = true;
    final bool isLeftRightEnabled = true;

    final List<Map<String, dynamic>> keys = [
      {'id': 1, 'label': tabLabel},
      {'id': 2, 'label': altLabel},
      {'id': 3, 'label': spcLabel},
      {'id': 4, 'label': shiftLabel},
      {'id': 5, 'label': enterLabel},
      {'id': 6, 'label': cLabel},
      {'id': 7, 'label': hLabel},
      {'id': 8, 'label': yLabel},
      {'id': 9, 'label': nLabel},
      {'id': 10, 'label': submissionLabel},
    ];

    if (widget.isF4Enabled) {
      keys.add({'id': 11, 'label': f4Label});
    }

    keys.add({'id': 18, 'label': f8Label});

    keys.add({'id': 12, 'label': f11Label});

    if (isUpDownEnabled) {
      keys.add({'id': 13, 'label': upLabel});
      keys.add({'id': 14, 'label': downLabel});
    }

    if (isLeftRightEnabled) {
      keys.add({'id': 16, 'label': leftLabel});
      keys.add({'id': 17, 'label': rightLabel});
    }

    return FadeTransition(
      opacity: _fadeAnimation,
      child: Material(
        type: MaterialType.transparency,
        child: Stack(
          children: [
            Positioned(
              left: leftPos,
              top: topPos,
              child: SizedBox(
                key: _panelKey,
                height: panelHeight,
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    SizedBox(width: spacing),
                    _ToggleButton(
                      isExpanded: _isExpanded,
                      width: btnWidth,
                      height: btnHeight,
                      borderRadius: borderRadius,
                      fontSize: fontSize,
                      scale: scale,
                      onTap: () {
                        if (!mounted) return;
                        setState(() {
                          _isExpanded = !_isExpanded;
                          if (_isExpanded) {
                            _expandController.forward();
                          } else {
                            _expandController.reverse();
                          }
                        });
                      },
                    ),
                    AnimatedBuilder(
                      animation: _expandAnimation,
                      builder: (context, child) {
                        final double animValue = _expandAnimation.value;
                        if (animValue == 0.0) {
                          return const SizedBox.shrink();
                        }

                        final double maxBarWidth =
                            math.min(387.0 * scale, availableBarWidth);
                        final double currentWidth = animValue * maxBarWidth;
                        final double currentMargin = animValue * spacing;

                        return Container(
                          width: currentWidth,
                          height: btnHeight + (6.0 * scale),
                          margin: EdgeInsets.only(left: currentMargin),
                          clipBehavior: Clip.hardEdge,
                          decoration: const BoxDecoration(
                            color: Colors.transparent,
                          ),
                          alignment: Alignment.topLeft,
                          child: ShaderMask(
                            shaderCallback: (Rect bounds) {
                              return LinearGradient(
                                begin: Alignment.centerLeft,
                                end: Alignment.centerRight,
                                colors: const [
                                  Colors.white,
                                  Colors.white,
                                  Colors.white,
                                  Colors.white,
                                  Colors.transparent,
                                ],
                                stops: const [0.0, 0.2, 0.8, 0.90, 1.0],
                              ).createShader(bounds);
                            },
                            blendMode: BlendMode.dstIn,
                            child: Padding(
                              padding: EdgeInsets.only(bottom: 6.0 * scale),
                              child: ListView.separated(
                                scrollDirection: Axis.horizontal,
                                physics: const BouncingScrollPhysics(),
                                padding: EdgeInsets.only(right: 12.0 * scale),
                                itemCount: keys.length,
                                separatorBuilder: (context, index) =>
                                    SizedBox(width: spacing),
                                itemBuilder: (context, index) {
                                  final keyItem = keys[index];
                                  return _KeyButton(
                                    label: keyItem['label'],
                                    onTap: () => _sendKeyPress(keyItem['id']),
                                    width: btnWidth,
                                    height: btnHeight,
                                    borderRadius: borderRadius,
                                    fontSize: fontSize,
                                    scale: scale,
                                  );
                                },
                              ),
                            ),
                          ),
                        );
                      },
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ignore: unused_element
class _SettingsButton extends StatefulWidget {
  final VoidCallback onTap;
  final double width;
  final double height;
  final double borderRadius;
  final double scale;

  const _SettingsButton({
    Key? key,
    required this.onTap,
    required this.width,
    required this.height,
    required this.borderRadius,
    required this.scale,
  }) : super(key: key);

  @override
  State<_SettingsButton> createState() => _SettingsButtonState();
}

class _SettingsButtonState extends State<_SettingsButton> {
  bool _isPressed = false;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: widget.onTap,
      child: Listener(
        onPointerDown: (_) {
          if (mounted) setState(() => _isPressed = true);
        },
        onPointerUp: (_) {
          if (mounted) setState(() => _isPressed = false);
        },
        onPointerCancel: (_) {
          if (mounted) setState(() => _isPressed = false);
        },
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 60),
          width: widget.width,
          height: widget.height,
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(widget.borderRadius),
            border: Border.all(
              color: _isPressed
                  ? Colors.white.withOpacity(0.35)
                  : Colors.white.withOpacity(0.12),
              width: 0.8 * widget.scale,
            ),
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: _isPressed
                  ? [
                      const Color(0xFF161616).withOpacity(0.85),
                      const Color(0xFF0A0A0A).withOpacity(0.85),
                    ]
                  : [
                      const Color(0xFF2E2E2E).withOpacity(0.75),
                      const Color(0xFF161616).withOpacity(0.75),
                    ],
            ),
            boxShadow: _isPressed
                ? []
                : [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.35),
                      offset: Offset(0, 1.5 * widget.scale),
                      blurRadius: 1.5 * widget.scale,
                    ),
                  ],
          ),
          alignment: Alignment.center,
          child: AnimatedScale(
            duration: const Duration(milliseconds: 60),
            scale: _isPressed ? 0.92 : 1.0,
            child: Icon(
              Icons.settings,
              color: _isPressed
                  ? Colors.white.withOpacity(0.70)
                  : Colors.white.withOpacity(0.85),
              size: 14.0 * widget.scale,
            ),
          ),
        ),
      ),
    );
  }
}

class _ToggleButton extends StatefulWidget {
  final bool isExpanded;
  final VoidCallback onTap;
  final double width;
  final double height;
  final double borderRadius;
  final double fontSize;
  final double scale;

  const _ToggleButton({
    Key? key,
    required this.isExpanded,
    required this.onTap,
    required this.width,
    required this.height,
    required this.borderRadius,
    required this.fontSize,
    required this.scale,
  }) : super(key: key);

  @override
  State<_ToggleButton> createState() => _ToggleButtonState();
}

class _ToggleButtonState extends State<_ToggleButton> {
  bool _isPressed = false;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: widget.onTap,
      child: Listener(
        onPointerDown: (_) {
          if (mounted) setState(() => _isPressed = true);
        },
        onPointerUp: (_) {
          if (mounted) setState(() => _isPressed = false);
        },
        onPointerCancel: (_) {
          if (mounted) setState(() => _isPressed = false);
        },
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 60),
          width: widget.width,
          height: widget.height,
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(widget.borderRadius),
            border: Border.all(
              color: _isPressed
                  ? Colors.white.withOpacity(0.35)
                  : Colors.white.withOpacity(0.12),
              width: 0.8 * widget.scale,
            ),
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: _isPressed
                  ? [
                      const Color(0xFF161616).withOpacity(0.85),
                      const Color(0xFF0A0A0A).withOpacity(0.85),
                    ]
                  : [
                      const Color(0xFF2E2E2E).withOpacity(0.75),
                      const Color(0xFF161616).withOpacity(0.75),
                    ],
            ),
            boxShadow: _isPressed
                ? []
                : [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.35),
                      offset: Offset(0, 1.5 * widget.scale),
                      blurRadius: 1.5 * widget.scale,
                    ),
                  ],
          ),
          alignment: Alignment.center,
          child: AnimatedScale(
            duration: const Duration(milliseconds: 60),
            scale: _isPressed ? 0.92 : 1.0,
            child: Text(
              widget.isExpanded ? "<<" : ">>",
              style: TextStyle(
                color: _isPressed
                    ? Colors.white.withOpacity(0.70)
                    : Colors.white.withOpacity(0.85),
                fontSize: widget.fontSize,
                fontWeight: FontWeight.bold,
                decoration: TextDecoration.none,
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _KeyButton extends StatefulWidget {
  final String label;
  final VoidCallback onTap;
  final double width;
  final double height;
  final double borderRadius;
  final double fontSize;
  final double scale;

  const _KeyButton({
    Key? key,
    required this.label,
    required this.onTap,
    required this.width,
    required this.height,
    required this.borderRadius,
    required this.fontSize,
    required this.scale,
  }) : super(key: key);

  @override
  State<_KeyButton> createState() => _KeyButtonState();
}

class _KeyButtonState extends State<_KeyButton> {
  bool _isPressed = false;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: widget.onTap,
      child: Listener(
        onPointerDown: (_) {
          if (mounted) setState(() => _isPressed = true);
        },
        onPointerUp: (_) {
          if (mounted) setState(() => _isPressed = false);
        },
        onPointerCancel: (_) {
          if (mounted) setState(() => _isPressed = false);
        },
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 60),
          width: widget.width,
          height: widget.height,
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(widget.borderRadius),
            border: Border.all(
              color: _isPressed
                  ? Colors.white.withOpacity(0.35)
                  : Colors.white.withOpacity(0.12),
              width: 0.8 * widget.scale,
            ),
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: _isPressed
                  ? [
                      const Color(0xFF161616).withOpacity(0.85),
                      const Color(0xFF0A0A0A).withOpacity(0.85),
                    ]
                  : [
                      const Color(0xFF2E2E2E).withOpacity(0.75),
                      const Color(0xFF161616).withOpacity(0.75),
                    ],
            ),
            boxShadow: _isPressed
                ? []
                : [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.35),
                      offset: Offset(0, 1.5 * widget.scale),
                      blurRadius: 1.5 * widget.scale,
                    ),
                  ],
          ),
          alignment: Alignment.center,
          child: AnimatedScale(
            duration: const Duration(milliseconds: 60),
            scale: _isPressed ? 0.92 : 1.0,
            child: Text(
              widget.label,
              style: TextStyle(
                color: _isPressed
                    ? Colors.white.withOpacity(0.75)
                    : Colors.white.withOpacity(0.9),
                fontSize: widget.fontSize,
                fontWeight: FontWeight.w600,
                decoration: TextDecoration.none,
              ),
            ),
          ),
        ),
      ),
    );
  }
}
