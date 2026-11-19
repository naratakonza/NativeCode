import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// หน้าจอ Game Loading Screen (SA Architecture Bottom HUD)
class GameLoadingScreen extends StatefulWidget {
  const GameLoadingScreen({super.key, this.isVisible = true});

  final bool isVisible;
  static final isLoadingActive = ValueNotifier<bool>(false);

  @override
  State<GameLoadingScreen> createState() => _GameLoadingScreenState();
}

class _GameLoadingScreenState extends State<GameLoadingScreen>
    with TickerProviderStateMixin {
  late final AnimationController _spinnerController;
  late final AnimationController _progressController;
  late final AnimationController _pulseController;
  late final AnimationController _fadeController;
  late final Animation<double> _pulseAnimation;
  late final Animation<double> _fadeAnimation;

  static const platform = MethodChannel('flutter_loadingscreen');
  static const _initialStatus =
      'กำลังเตรียมความพร้อมของเมือง และทรัพยากรเกม...';
  bool _isVisible = false;
  bool _finishing = false;
  String _loadingStatus = _initialStatus;
  int _cycle = 0;

  double get _displayProgress => _progressController.value;

  @override
  void initState() {
    super.initState();
    _spinnerController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1400),
    );
    _progressController = AnimationController(vsync: this);
    _pulseController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1500),
    );
    _fadeController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 500),
    );
    _pulseAnimation = Tween<double>(
      begin: 0.60,
      end: 1.0,
    ).animate(CurveTween(curve: Curves.easeInOut).animate(_pulseController));
    _fadeAnimation = CurveTween(
      curve: Curves.easeInOutCubic,
    ).animate(_fadeController);

    platform.setMethodCallHandler(_onNativeMessage);
    if (widget.isVisible) {
      _startLoadingAnimation();
    }
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) _notify('ready');
    });
  }

  Future<void> _notify(String method) async {
    try {
      await platform.invokeMethod<void>(method);
    } on MissingPluginException {
      // The widget can also run on its own in Flutter.
    }
  }

  Future<void> _onNativeMessage(MethodCall call) async {
    switch (call.method) {
      case 'show':
        _startLoadingAnimation();
        break;
      case 'hide':
        if (!_finishing) _stopLoadingAnimation();
        break;
      case 'updateProgress':
        final value = call.arguments;
        if (value is int && _isVisible && !_finishing) {
          if (value >= 100) {
            _finishLoading();
          } else {
            final target = math.max(_displayProgress, value.clamp(0, 99) / 100);
            _progressController.animateTo(
              target,
              duration: const Duration(milliseconds: 500),
            );
          }
        }
        break;
      case 'updateStatusText':
        final value = call.arguments;
        if (value is String && mounted) setState(() => _loadingStatus = value);
        break;
    }
  }

  void _startLoadingAnimation() {
    if (!mounted) return;
    ++_cycle;
    _finishing = false;
    _progressController.reset();
    GameLoadingScreen.isLoadingActive.value = true;
    setState(() {
      _loadingStatus = _initialStatus;
      _isVisible = true;
    });
    _fadeController.forward();
    _spinnerController.repeat();
    _pulseController.repeat(reverse: true);
    _progressController.animateTo(
      0.95,
      duration: const Duration(seconds: 15),
      curve: Curves.easeOutCubic,
    );
  }

  Future<void> _finishLoading() async {
    _finishing = true;
    final cycle = _cycle;
    try {
      await _progressController
          .animateTo(
            1,
            duration: const Duration(milliseconds: 150),
            curve: Curves.fastOutSlowIn,
          )
          .orCancel;
      await Future<void>.delayed(const Duration(milliseconds: 250));
      if (mounted && cycle == _cycle) await _stopLoadingAnimation();
    } on TickerCanceled {
      // A later show or disposal owns the animation.
    }
  }

  Future<void> _stopLoadingAnimation() async {
    if (!mounted || !_isVisible) return;
    final cycle = _cycle;
    try {
      await _fadeController.reverse().orCancel;
    } on TickerCanceled {
      return;
    }
    if (!mounted || cycle != _cycle) return;
    _spinnerController.stop();
    _pulseController.stop();
    _progressController.stop();
    _finishing = false;
    GameLoadingScreen.isLoadingActive.value = false;
    setState(() => _isVisible = false);
    await _notify('onLoadingComplete');
  }

  @override
  void didUpdateWidget(GameLoadingScreen oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (!oldWidget.isVisible && widget.isVisible) {
      _startLoadingAnimation();
    } else if (oldWidget.isVisible && !widget.isVisible) {
      _stopLoadingAnimation();
    }
  }

  @override
  void dispose() {
    platform.setMethodCallHandler(null);
    GameLoadingScreen.isLoadingActive.value = false;
    _spinnerController.dispose();
    _progressController.dispose();
    _pulseController.dispose();
    _fadeController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (!_isVisible) {
      return const SizedBox.shrink();
    }

    final mediaQuery = MediaQuery.of(context);
    final size = mediaQuery.size;
    final viewPadding = mediaQuery.viewPadding;

    // Responsive Scale ตามความสูงหน้าจอแนวนอน (Landscape Mode)
    final scale = (size.height / 450.0).clamp(0.70, 1.30);

    // แก้ไขปัญหาความไม่สมดุลของจอที่มีรูกล้อง (Punch Hole Cutout):
    // ดึงค่า Inset สูงสุดระหว่างซ้ายและขวามาใช้ทั้งสองฝั่ง เพื่อให้ระยะห่างซ้าย-ขวาเท่ากันเป๊ะ 100%
    final maxHorizontalCutout = math.max(viewPadding.left, viewPadding.right);
    final horizontalMargin = maxHorizontalCutout + (36.0 * scale);
    final bottomMargin = viewPadding.bottom + (20.0 * scale);

    return Scaffold(
      backgroundColor: Colors.transparent,
      body: FadeTransition(
        opacity: _fadeAnimation,
        child: AnimatedBuilder(
          animation: Listenable.merge([
            _spinnerController,
            _progressController,
            _pulseController,
          ]),
          child: Stack(
            fit: StackFit.expand,
            children: [
              // 1. ภาพพื้นหลังหลัก backgroud01.png
              Positioned.fill(
                child: Image.asset(
                  "assets/backgroud01.png",
                  fit: BoxFit.cover,
                  cacheWidth: 1920,
                  errorBuilder: (context, error, stackTrace) =>
                      Container(color: const Color(0xFF0A0F1D)),
                ),
              ),

              // 2. Cinematic Bottom Gradient Vignette เพื่อขับเน้นข้อความและแถบโหลดให้อ่านชัดเจน 100%
              Positioned(
                left: 0,
                right: 0,
                bottom: 0,
                height: 200.0 * scale,
                child: Container(
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                      begin: Alignment.topCenter,
                      end: Alignment.bottomCenter,
                      colors: [
                        Colors.transparent,
                        Colors.black.withAlpha(115),
                        Colors.black.withAlpha(217),
                      ],
                      stops: const [0.0, 0.45, 1.0],
                    ),
                  ),
                ),
              ),
            ],
          ),
          builder: (context, staticBackground) {
            final currentPercent = (_displayProgress * 100).round().clamp(
                  0,
                  100,
                );

            return SizedBox.expand(
              child: Stack(
                fit: StackFit.expand,
                children: [
                  staticBackground!,

                  // 3. Bottom HUD Panel: แถบสถานะความคืบหน้าด้านล่างหน้าจอ
                  Align(
                    alignment: Alignment.bottomCenter,
                    child: Padding(
                      padding: EdgeInsets.only(
                        left: horizontalMargin,
                        right: horizontalMargin,
                        bottom: bottomMargin,
                      ),
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        crossAxisAlignment: CrossAxisAlignment.stretch,
                        children: [
                          // แถวข้อมูล Header (Spinner + ข้อความฝั่งซ้าย และ เปอร์เซ็นต์ฝั่งขวา)
                          Row(
                            crossAxisAlignment: CrossAxisAlignment.end,
                            children: [
                              // ฝั่งซ้าย: Spinner หมุนวน + ข้อความหัวข้อ + รายละเอียด (ใช้ Expanded ป้องกัน Text Overflow)
                              Expanded(
                                child: Row(
                                  crossAxisAlignment: CrossAxisAlignment.center,
                                  children: [
                                    _buildMiniSpinner(scale),
                                    SizedBox(width: 12.0 * scale),
                                    Expanded(
                                      child: Column(
                                        crossAxisAlignment:
                                            CrossAxisAlignment.start,
                                        mainAxisSize: MainAxisSize.min,
                                        children: [
                                          Text(
                                            "กำลังโหลดข้อมูล",
                                            style: TextStyle(
                                              fontFamily: 'NotoSans',
                                              fontFamilyFallback: const [
                                                'Kanit',
                                              ],
                                              fontSize: 14.5 * scale,
                                              fontWeight: FontWeight.w700,
                                              color: Colors.white,
                                              letterSpacing: 0.4,
                                              shadows: const [
                                                Shadow(
                                                  blurRadius: 4,
                                                  color: Colors.black87,
                                                  offset: Offset(0, 1),
                                                ),
                                              ],
                                            ),
                                          ),
                                          SizedBox(height: 2.0 * scale),
                                          Text(
                                            _loadingStatus,
                                            maxLines: 1,
                                            overflow: TextOverflow.ellipsis,
                                            style: TextStyle(
                                              fontFamily: 'NotoSans',
                                              fontFamilyFallback: const [
                                                'Kanit',
                                              ],
                                              fontSize: 11.0 * scale,
                                              fontWeight: FontWeight.w400,
                                              color: Colors.white.withAlpha(
                                                178,
                                              ),
                                              letterSpacing: 0.2,
                                            ),
                                          ),
                                        ],
                                      ),
                                    ),
                                  ],
                                ),
                              ),

                              // ช่องว่างคั่นระหว่างชุดข้อความซ้ายและตัวเลขขวา
                              SizedBox(width: 20.0 * scale),

                              // ฝั่งขวา: เปอร์เซ็นต์ + ป้ายกำกับสถานะ (จัดวางสมดุล ไม่เบียดขอบ)
                              Column(
                                crossAxisAlignment: CrossAxisAlignment.end,
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  Row(
                                    mainAxisSize: MainAxisSize.min,
                                    crossAxisAlignment:
                                        CrossAxisAlignment.baseline,
                                    textBaseline: TextBaseline.alphabetic,
                                    children: [
                                      Text(
                                        "$currentPercent",
                                        style: TextStyle(
                                          fontFamily: 'Rajdhani',
                                          fontSize: 21.0 * scale,
                                          fontWeight: FontWeight.w700,
                                          color: Colors.white,
                                          letterSpacing: 0.5,
                                          shadows: const [
                                            Shadow(
                                              blurRadius: 6,
                                              color: Colors.black87,
                                              offset: Offset(0, 1),
                                            ),
                                          ],
                                        ),
                                      ),
                                      SizedBox(width: 3.0 * scale),
                                      Text(
                                        "%",
                                        style: TextStyle(
                                          fontFamily: 'Rajdhani',
                                          fontSize: 14.0 * scale,
                                          fontWeight: FontWeight.w700,
                                          color: const Color(0xFFFFD34D),
                                        ),
                                      ),
                                    ],
                                  ),
                                  SizedBox(height: 1.5 * scale),
                                  Text(
                                    "ดาวน์โหลดแล้ว",
                                    style: TextStyle(
                                      fontFamily: 'NotoSans',
                                      fontFamilyFallback: const ['Kanit'],
                                      fontSize: 10.5 * scale,
                                      fontWeight: FontWeight.w400,
                                      color: Colors.white.withAlpha(140),
                                      letterSpacing: 0.2,
                                    ),
                                  ),
                                ],
                              ),
                            ],
                          ),

                          SizedBox(height: 10.0 * scale),

                          // แถบหลอด Progress Bar แนวนอนเต็มความกว้าง (ขอบมนนิดๆ สไตล์ Tactical HUD)
                          _buildProgressBar(scale),
                        ],
                      ),
                    ),
                  ),
                ],
              ),
            );
          },
        ),
      ),
    );
  }

  /// ไอคอนวงแหวนหมุนโหลดสไตล์ Sci-Fi นีออน (Mini Spinning Loader)
  Widget _buildMiniSpinner(double scale) {
    return RepaintBoundary(
      child: SizedBox(
        width: 20.0 * scale,
        height: 20.0 * scale,
        child: AnimatedBuilder(
          animation: _spinnerController,
          builder: (context, child) {
            return CustomPaint(
              painter: _MiniSpinnerPainter(
                progress: _spinnerController.value,
                color: const Color(0xFFFFD34D),
                strokeWidth: 2.2 * scale,
              ),
            );
          },
        ),
      ),
    );
  }

  /// แถบหลอดดาวน์โหลดแนวนอน (ขอบเหลี่ยมคมกริบ 100% สไตล์ Sharp Military/Tactical HUD)
  Widget _buildProgressBar(double scale) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final totalWidth = constraints.maxWidth;
        final fillWidth = (totalWidth * _displayProgress).clamp(
          0.0,
          totalWidth,
        );
        final height = 6.0 * scale; // เพรียว คม ไม่หนาเทอะทะ
        const barRadius = BorderRadius.zero; // ขอบเหลี่ยมคมกริบ 100% ไม่มนเลย

        return Container(
          height: height,
          width: totalWidth,
          decoration: BoxDecoration(
            color: const Color(0xFF0F1923).withAlpha(217),
            borderRadius: barRadius,
            border: Border.all(
              color: Colors.white.withAlpha(31),
              width: 0.8 * scale,
            ),
          ),
          child: Stack(
            children: [
              // หลอดเนื้อในสีเขียวนีออนมิ้นต์ ขอบเหลี่ยมคมกริบ พร้อม Glow เบาๆ คมชัด
              Container(
                width: fillWidth,
                height: height,
                decoration: BoxDecoration(
                  borderRadius: barRadius,
                  gradient: const LinearGradient(
                    colors: [
                      Color(0xFFE5A900),
                      Color(0xFFFFD34D),
                      Color(0xFFFFED99),
                    ],
                  ),
                  boxShadow: [
                    BoxShadow(
                      color: const Color(
                        0xFFFFD34D,
                      ).withOpacity(0.60 * _pulseAnimation.value),
                      blurRadius: 6.0 * scale,
                      spreadRadius: 0.0,
                    ),
                  ],
                ),
              ),
            ],
          ),
        );
      },
    );
  }
}

/// CustomPainter สำหรับวาด Mini Spinner วงล้อหมุนโหลด
class _MiniSpinnerPainter extends CustomPainter {
  final double progress;
  final Color color;
  final double strokeWidth;

  const _MiniSpinnerPainter({
    required this.progress,
    required this.color,
    required this.strokeWidth,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = (size.width - strokeWidth) / 2;

    // รอยร่องพื้นหลังบางๆ
    final bgPaint = Paint()
      ..color = color.withAlpha(46)
      ..style = PaintingStyle.stroke
      ..strokeWidth = strokeWidth * 0.7;
    canvas.drawCircle(center, radius, bgPaint);

    // Arc เส้นโค้งนีออนหมุนวน
    final paint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = strokeWidth
      ..strokeCap = StrokeCap.round;

    final startAngle = progress * 2 * math.pi;
    const sweepAngle = math.pi * 0.95;
    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      startAngle,
      sweepAngle,
      false,
      paint,
    );
  }

  @override
  bool shouldRepaint(covariant _MiniSpinnerPainter oldDelegate) {
    return oldDelegate.progress != progress;
  }
}
