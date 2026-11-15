import 'dart:async';
import 'dart:math' as math;

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

class SampSpawnMenu extends StatelessWidget {
  const SampSpawnMenu({super.key});

  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder<bool>(
      valueListenable: SpawnState.isSpawnMenuVisible,
      builder: (context, visible, child) {
        if (!visible) {
          return const SizedBox.shrink();
        }

        return LayoutBuilder(builder: (context, constraints) {
          final width = constraints.maxWidth;
          final height = constraints.maxHeight;
          final safe = MediaQuery.viewPaddingOf(context);
          final scale = (width / 970).clamp(0.9, 1.5).toDouble();
          final safeWidth = width - safe.horizontal;
          final panelWidth = math.min(310 * scale, safeWidth - 16);
          final panelHeight = math.max(52 * scale, 52.0);
          final bottom = height - safe.bottom - 23 * scale;

          if (panelWidth <= 0 || bottom - panelHeight < safe.top) {
            return const SizedBox.shrink();
          }

          final rect = Rect.fromLTWH(
            safe.left + (safeWidth - panelWidth) / 2,
            bottom - panelHeight,
            panelWidth,
            panelHeight,
          );
          return Stack(children: [
            Positioned.fromRect(
              rect: rect,
              child: _SpawnPanel(rect: rect, scale: scale),
            ),
          ]);
        });
      },
    );
  }
}

class _SpawnPanel extends StatefulWidget {
  const _SpawnPanel({required this.rect, required this.scale});

  final Rect rect;
  final double scale;

  @override
  State<_SpawnPanel> createState() => _SpawnPanelState();
}

class _SpawnPanelState extends State<_SpawnPanel>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  late final Animation<double> _fade;
  late final Animation<Offset> _slide;
  String? _activeAction;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 450),
    );
    _fade = _controller.drive(CurveTween(curve: Curves.easeOut));
    _slide = _controller
        .drive(CurveTween(curve: Curves.easeOutQuint))
        .drive(Tween(begin: const Offset(0, 1.2), end: Offset.zero));
    _controller.forward().then((_) {
      _publishHitRegion();
    });
    HardwareKeyboard.instance.addHandler(_handleKey);
  }

  @override
  void didUpdateWidget(covariant _SpawnPanel oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.rect != widget.rect && _controller.isCompleted) {
      _publishHitRegion();
    }
  }

  void _publishHitRegion() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted && SpawnState.isSpawnMenuVisible.value) {
        SpawnBridge.sendHitRegion(widget.rect);
      }
    });
  }

  bool _handleKey(KeyEvent event) {
    if (!SpawnState.isSpawnMenuVisible.value) {
      return false;
    }

    final action = switch (event.logicalKey) {
      LogicalKeyboardKey.arrowLeft => 'PREV_SKIN',
      LogicalKeyboardKey.arrowRight => 'NEXT_SKIN',
      LogicalKeyboardKey.shiftLeft ||
      LogicalKeyboardKey.shiftRight =>
        'SPAWN_PLAYER',
      _ => null,
    };
    if (action == null) {
      return false;
    }
    if (event is KeyDownEvent) {
      setState(() => _activeAction = action);
      SpawnBridge.sendSpawnAction(action);
    } else if (event is KeyUpEvent && _activeAction == action) {
      setState(() => _activeAction = null);
    }
    return true;
  }

  @override
  void dispose() {
    HardwareKeyboard.instance.removeHandler(_handleKey);
    SpawnBridge.sendHitRegion(Rect.zero);
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final scale = widget.scale;
    final radius = BorderRadius.circular(12 * scale);
    return FadeTransition(
      opacity: _fade,
      child: SlideTransition(
        position: _slide,
        child: DecoratedBox(
          decoration: BoxDecoration(
            borderRadius: radius,
            boxShadow: [
              BoxShadow(
                color: Colors.black.withOpacity(0.65),
                blurRadius: 24 * scale,
                spreadRadius: 3 * scale,
                offset: Offset(0, 8 * scale),
              ),
            ],
          ),
          child: Material(
            type: MaterialType.transparency,
            child: Ink(
              decoration: BoxDecoration(
                borderRadius: radius,
                border: Border.all(color: Colors.white.withOpacity(0.08)),
                gradient: const LinearGradient(
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                  colors: [Color(0xE60D0F10), Color(0xE61F2428)],
                ),
              ),
              child: Padding(
                padding: EdgeInsets.symmetric(horizontal: 8 * scale),
                child: Row(
                  children: [
                    _button('PREV_SKIN', 'ก่อนหน้า', 'เปลี่ยนตัวละคร',
                        Icons.arrow_back_ios_new_rounded),
                    _divider(),
                    _button('SPAWN_PLAYER', 'เกิด', 'เข้าสู่เกม',
                        Icons.check_rounded),
                    _divider(),
                    _button('NEXT_SKIN', 'ถัดไป', 'เปลี่ยนตัวละคร',
                        Icons.arrow_forward_ios_rounded),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _divider() {
    return VerticalDivider(
      color: const Color(0x26FFB800),
      width: 1,
      thickness: 1,
      indent: 10,
      endIndent: 10,
    );
  }

  Widget _button(String action, String title, String caption, IconData icon) {
    final isSpawn = action == 'SPAWN_PLAYER';
    final active = _activeAction == action;
    final color = isSpawn ? const Color(0xFF00C853) : const Color(0xFFFFB800);
    final scale = widget.scale;
    return Expanded(
      child: Semantics(
        button: true,
        child: InkWell(
          borderRadius: BorderRadius.circular(8 * scale),
          highlightColor: color.withOpacity(0.12),
          onTap: () => SpawnBridge.sendSpawnAction(action),
          child: AnimatedScale(
            duration: const Duration(milliseconds: 100),
            scale: active ? 0.925 : 1.0,
            child: Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                if (action != 'NEXT_SKIN') _icon(icon, active, color),
                Flexible(
                  child: Padding(
                    padding: EdgeInsets.symmetric(horizontal: 4 * scale),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      crossAxisAlignment: action == 'NEXT_SKIN'
                          ? CrossAxisAlignment.end
                          : CrossAxisAlignment.start,
                      children: [
                        Text(
                          title,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: TextStyle(
                            color: Colors.white,
                            fontFamily: 'Kanit',
                            fontSize: 13 * scale,
                            fontWeight: FontWeight.w700,
                          ),
                        ),
                        Text(
                          caption,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: TextStyle(
                            color: Colors.white.withOpacity(0.62),
                            fontFamily: 'NotoSans',
                            fontSize: 9 * scale,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                if (action == 'NEXT_SKIN') _icon(icon, active, color),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _icon(IconData icon, bool active, Color color) {
    final scale = widget.scale;
    return AnimatedContainer(
      duration: const Duration(milliseconds: 150),
      width: 20 * scale,
      height: 20 * scale,
      decoration: BoxDecoration(
        color: active ? color : Colors.white.withOpacity(0.06),
        shape: BoxShape.circle,
      ),
      child: Icon(
        icon,
        color: active ? Colors.black : Colors.white,
        size: 11 * scale,
      ),
    );
  }
}
