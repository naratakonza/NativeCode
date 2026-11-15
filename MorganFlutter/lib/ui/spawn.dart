import 'dart:async';
import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class SpawnState {
  static final isSpawnMenuVisible = ValueNotifier<bool>(false);
}

class SpawnBridge {
  static const _channel = MethodChannel('flutter_spawn');

  static void init() {
    _channel.setMethodCallHandler((call) async {
      final visible = call.arguments;
      if (call.method == 'toggleSpawnMenu' && visible is bool) {
        SpawnState.isSpawnMenuVisible.value = visible;
      }
    });
    WidgetsBinding.instance.addPostFrameCallback((_) {
      unawaited(_invoke('ready'));
    });
  }

  static void sendSpawnAction(String action) {
    unawaited(_invoke('onSpawnAction', {'action': action}));
  }

  static void sendHitRegion(Rect rect) {
    unawaited(_invoke('hitRegion', {
      'x': rect.left,
      'y': rect.top,
      'width': rect.width,
      'height': rect.height,
    }));
  }

  static Future<void> _invoke(String method, [Object? arguments]) async {
    try {
      await _channel.invokeMethod<bool>(method, arguments);
    } on MissingPluginException {
      // The menu is also hosted in the standalone Flutter module.
    } on PlatformException catch (error) {
      debugPrint('flutter_spawn $method: ${error.code}');
    }
  }
}

// Same font choice and fallback as Thunder's Thai UI.
const _spawnTextStyle = TextStyle(
  fontFamily: 'Sarabun',
  fontFamilyFallback: ['sans-serif'],
);

class SampSpawnMenu extends StatefulWidget {
  const SampSpawnMenu({Key? key}) : super(key: key);

  @override
  State<SampSpawnMenu> createState() => _SampSpawnMenuState();
}

class _SampSpawnMenuState extends State<SampSpawnMenu> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<Offset> _slideAnimation;
  late Animation<double> _fadeAnimation;
  final ValueNotifier<String?> _activeAction = ValueNotifier<String?>(null);
  final GlobalKey _panelKey = GlobalKey();
  Rect? _lastHitRegion;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 450),
    );

    _slideAnimation = Tween<Offset>(
      begin: const Offset(0, 1.2), // เริ่มจากล่างจอ
      end: Offset.zero,            // เลื่อนขึ้นมาที่เดิม
    ).animate(CurvedAnimation(
      parent: _controller,
      curve: Curves.easeOutQuint,
    ));

    _fadeAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _controller,
      curve: Curves.easeOut,
    ));

    _controller.addStatusListener((status) {
      if (status == AnimationStatus.completed) {
        _reportHitRegion();
      } else if (status == AnimationStatus.dismissed) {
        setState(() {});
      }
    });
    SpawnState.isSpawnMenuVisible.addListener(_handleVisibilityChange);

    if (SpawnState.isSpawnMenuVisible.value) {
      _controller.forward();
    }

    HardwareKeyboard.instance.addHandler(_hardwareKeyHandler);
  }

  bool _hardwareKeyHandler(KeyEvent event) {
    if (!SpawnState.isSpawnMenuVisible.value) return false;

    if (event is KeyDownEvent) {
      if (event.logicalKey == LogicalKeyboardKey.arrowLeft) {
        _activeAction.value = 'PREV_SKIN';
        SpawnBridge.sendSpawnAction('PREV_SKIN');
        return true;
      } else if (event.logicalKey == LogicalKeyboardKey.arrowRight) {
        _activeAction.value = 'NEXT_SKIN';
        SpawnBridge.sendSpawnAction('NEXT_SKIN');
        return true;
      } else if (event.logicalKey == LogicalKeyboardKey.shiftLeft ||
                 event.logicalKey == LogicalKeyboardKey.shiftRight) {
        _activeAction.value = 'SPAWN_PLAYER';
        SpawnBridge.sendSpawnAction('SPAWN_PLAYER');
        return true;
      }
    } else if (event is KeyUpEvent) {
      if (event.logicalKey == LogicalKeyboardKey.arrowLeft && _activeAction.value == 'PREV_SKIN') {
        _activeAction.value = null;
        return true;
      } else if (event.logicalKey == LogicalKeyboardKey.arrowRight && _activeAction.value == 'NEXT_SKIN') {
        _activeAction.value = null;
        return true;
      } else if ((event.logicalKey == LogicalKeyboardKey.shiftLeft ||
                  event.logicalKey == LogicalKeyboardKey.shiftRight) && _activeAction.value == 'SPAWN_PLAYER') {
        _activeAction.value = null;
        return true;
      }
    }
    return false;
  }

  void _reportHitRegion() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted || !SpawnState.isSpawnMenuVisible.value ||
          !_controller.isCompleted) {
        return;
      }

      final panel = _panelKey.currentContext?.findRenderObject();
      if (panel is! RenderBox || !panel.hasSize) {
        return;
      }

      final region = panel.localToGlobal(Offset.zero) & panel.size;
      if (region != _lastHitRegion) {
        _lastHitRegion = region;
        SpawnBridge.sendHitRegion(region);
      }
    });
  }

  void _handleVisibilityChange() {
    setState(() {});
    if (SpawnState.isSpawnMenuVisible.value) {
      _controller.forward();
    } else {
      _controller.reverse();
      _activeAction.value = null;
      _lastHitRegion = null;
      SpawnBridge.sendHitRegion(Rect.zero);
    }
  }

  @override
  void dispose() {
    HardwareKeyboard.instance.removeHandler(_hardwareKeyHandler);
    SpawnState.isSpawnMenuVisible.removeListener(_handleVisibilityChange);
    _controller.dispose();
    _activeAction.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final media = MediaQuery.of(context);
    final safeArea = media.viewPadding;
    final double usableWidth = media.size.width - safeArea.horizontal;
    final double scale = usableWidth / 970.0;
    if (_controller.isCompleted) {
      _reportHitRegion();
    }

    final double menuWidth = 310 * scale;
    final double bottomPadding = safeArea.bottom + 23 * scale;
    final double menuBorderRadius = 12.0 * scale;
    final double paddingVertical = 6.0 * scale;
    final double paddingHorizontal = 8.0 * scale;

    final menu = FadeTransition(
      opacity: _fadeAnimation,
      child: SlideTransition(
        position: _slideAnimation,
        child: Align(
          alignment: Alignment.bottomCenter,
          child: Padding(
            padding: EdgeInsets.only(
              left: safeArea.left,
              right: safeArea.right,
              bottom: bottomPadding,
            ),
            child: Container(
              key: _panelKey,
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(menuBorderRadius),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withOpacity(0.65),
                    blurRadius: 24 * scale,
                    spreadRadius: 3 * scale,
                    offset: Offset(0, 8 * scale),
                  ),
                  BoxShadow(
                    color: const Color(0xFFFFB800).withOpacity(0.02),
                    blurRadius: 16 * scale,
                    spreadRadius: 1 * scale,
                  ),
                ],
              ),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(menuBorderRadius),
                child: BackdropFilter(
                  filter: ImageFilter.blur(sigmaX: 12.0, sigmaY: 12.0),
                  child: Container(
                    width: menuWidth,
                    padding: EdgeInsets.symmetric(vertical: paddingVertical, horizontal: paddingHorizontal),
                    decoration: BoxDecoration(
                      borderRadius: BorderRadius.circular(menuBorderRadius),
                      border: Border.all(
                        color: Colors.white.withOpacity(0.08),
                        width: 1.0,
                      ),
                      gradient: const LinearGradient(
                        begin: Alignment.topLeft,
                        end: Alignment.bottomRight,
                        colors: [
                          Color(0xE60D0F10),
                          Color(0xE61F2428),
                        ],
                      ),
                    ),
                    child: IntrinsicHeight(
                      child: Row(
                        children: [
                          Expanded(
                            child: _SpawnButton(
                              action: 'PREV_SKIN',
                              mainText: 'ก่อนหน้า',
                              subText: 'เปลี่ยนตัวละคร',
                              icon: Icons.arrow_back_ios_new_rounded,
                              isIconLeft: true,
                              scale: scale,
                              activeAction: _activeAction,
                            ),
                          ),
                          _buildVerticalDivider(scale),
                          Expanded(
                            child: _SpawnButton(
                              action: 'SPAWN_PLAYER',
                              mainText: 'เกิด',
                              subText: 'เข้าสู่เกม',
                              icon: Icons.check_rounded,
                              isIconLeft: true,
                              scale: scale,
                              activeAction: _activeAction,
                            ),
                          ),
                          _buildVerticalDivider(scale),
                          Expanded(
                            child: _SpawnButton(
                              action: 'NEXT_SKIN',
                              mainText: 'ถัดไป',
                              subText: 'เปลี่ยนตัวละคร',
                              icon: Icons.arrow_forward_ios_rounded,
                              isIconLeft: false,
                              scale: scale,
                              activeAction: _activeAction,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );

    final visible = SpawnState.isSpawnMenuVisible.value;
    return Offstage(
      offstage: !visible && _controller.isDismissed,
      child: IgnorePointer(
        ignoring: !visible,
        child: Material(type: MaterialType.transparency, child: menu),
      ),
    );
  }

  Widget _buildVerticalDivider(double scale) {
    return VerticalDivider(
      color: const Color(0xFFFFB800).withOpacity(0.15),
      thickness: 0.8 * scale,
      indent: 6.0 * scale,
      endIndent: 6.0 * scale,
    );
  }
}

class _SpawnButton extends StatelessWidget {
  final String action;
  final String mainText;
  final String subText;
  final IconData icon;
  final bool isIconLeft;
  final double scale;
  final ValueNotifier<String?> activeAction;

  const _SpawnButton({
    Key? key,
    required this.action,
    required this.mainText,
    required this.subText,
    required this.icon,
    required this.isIconLeft,
    required this.scale,
    required this.activeAction,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final double buttonBorderRadius = 8.0 * scale;
    final double mainFontSize = 8.3 * scale;
    final double subFontSize = 5.4 * scale;
    final double iconSize = 8.6 * scale;
    final double iconBoxSize = 19.2 * scale;
    final double elementSpacing = 6.2 * scale;
    final double buttonPadding = 5.0 * scale;

    return MouseRegion(
      onEnter: (_) => activeAction.value = action,
      onExit: (_) {
        if (activeAction.value == action) activeAction.value = null;
      },
      child: GestureDetector(
        onTapDown: (_) => activeAction.value = action,
        onTapUp: (_) {
          if (activeAction.value == action) activeAction.value = null;
        },
        onTapCancel: () {
          if (activeAction.value == action) activeAction.value = null;
        },
        onTap: () => SpawnBridge.sendSpawnAction(action),
        child: InkWell(
          onTap: null,
          borderRadius: BorderRadius.circular(buttonBorderRadius),
          child: ValueListenableBuilder<String?>(
            valueListenable: activeAction,
            builder: (context, activeActionValue, child) {

              bool active = activeActionValue == action;
              bool isSpawn = action == 'SPAWN_PLAYER';

              final Color buttonBgColor = active
              ? (isSpawn
                  ? const Color(0xFF00C853).withOpacity(0.12)
                  : const Color(0xFFFFB800).withOpacity(0.08))
              : Colors.transparent;

              final Color hoveredColor = const Color(0xFFFFB800);
              final Color iconColor = active ? Colors.black : Colors.white.withOpacity(0.7);
              final Color iconBgColor = active
              ? (isSpawn ? const Color(0xFF00C853) : hoveredColor)
              : Colors.white.withOpacity(0.04);
              final Color mainTextColor = isSpawn ? Colors.white : (active ? Colors.white : Colors.white.withOpacity(0.7));
              final Color subTextColor = Colors.white.withOpacity(0.4);

              final textColumn = Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: isIconLeft ? CrossAxisAlignment.start : CrossAxisAlignment.end,
                children: [
                  Text(
                    mainText,
                    style: _spawnTextStyle.copyWith(
                      color: mainTextColor,
                      fontSize: mainFontSize,
                      fontWeight: FontWeight.bold,
                      letterSpacing: 0.26 * scale,
                    ),
                  ),
                  SizedBox(height: 1.0 * scale),
                  Text(
                    subText,
                    style: _spawnTextStyle.copyWith(
                      color: subTextColor,
                      fontSize: subFontSize,
                      fontWeight: FontWeight.w600,
                      letterSpacing: 0.105 * scale,
                    ),
                  ),
                ],
              );

              final circularIcon = AnimatedContainer(
                duration: const Duration(milliseconds: 150),
                width: iconBoxSize,
                height: iconBoxSize,
                decoration: BoxDecoration(
                  color: iconBgColor,
                  shape: BoxShape.circle,
                  boxShadow: active
                      ? [
                          BoxShadow(
                            color: isSpawn
                                ? const Color(0xFF00C853).withOpacity(0.35)
                                : hoveredColor.withOpacity(0.35),
                            blurRadius: 6.2 * scale,
                            spreadRadius: 1.05 * scale,
                          ),
                        ]
                      : null,
                ),
                child: Icon(
                  icon,
                  color: iconColor,
                  size: iconSize,
                ),
              );

              final fittedText = Flexible(
                child: FittedBox(
                  fit: BoxFit.scaleDown,
                  child: textColumn,
                ),
              );

              return Ink(
                padding: EdgeInsets.symmetric(vertical: buttonPadding, horizontal: buttonPadding),
                decoration: BoxDecoration(
                  color: buttonBgColor,
                  borderRadius: BorderRadius.circular(buttonBorderRadius),
                ),
                child: AnimatedScale(
                  scale: active ? 0.925 : 1.0,
                  duration: const Duration(milliseconds: 100),
                  curve: Curves.easeOutCubic,
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: isIconLeft
                        ? [circularIcon, SizedBox(width: elementSpacing), fittedText]
                        : [fittedText, SizedBox(width: elementSpacing), circularIcon],
                  ),
                ),
              );
            },
          ),
        ),
      ),
    );
  }
}
