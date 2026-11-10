import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';

class SampChat extends StatefulWidget {
  final int maxMessages;
  final bool showTimestamp;
  final int timestampStyle;

  const SampChat({
    super.key,
    this.maxMessages = 40,
    this.showTimestamp = true,
    this.timestampStyle = 0,
  });

  @override
  State<SampChat> createState() => SampChatState();
}

class SampChatState extends State<SampChat> with SingleTickerProviderStateMixin {
  final List<_ChatMessage> _messages = [];
  final ScrollController _scrollController = ScrollController();
  bool chatstatus = false;
  bool chatInputstatus = false;
  final TextEditingController _controller = TextEditingController();
  final FocusNode _focusNode = FocusNode();
  bool _isAtBottom = true;
  bool _visible = false;
  bool _sending = false;
  Rect? _hitRegion;
  List<String> history = [];
  int historyIndex = -1;
  late AnimationController _borderController;
  bool isHolding = false;
  double chatPosValue = 16.0;

  final ValueNotifier<bool> _isScrollUIActive = ValueNotifier(false);

  final ValueNotifier<bool> _showNewMessageButton = ValueNotifier(false);

  @override
  void initState() {
    super.initState();
    SampChatController.init(this);
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) unawaited(SampChatController.notify('ready'));
    });
    _borderController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 700),
    )..addStatusListener((status) {
        if (status == AnimationStatus.completed) _onHoldComplete();
      });

    _scrollController.addListener(() {
      if (!_scrollController.hasClients) return;
      bool atBottom = _scrollController.offset <= 20;
      if (atBottom != _isAtBottom) {
        setState(() {
          _isAtBottom = atBottom;
          if (atBottom) _showNewMessageButton.value = false;
        });
      }
    });
  }

  @override
  void dispose() {
    _borderController.dispose();
    _scrollController.dispose();
    _controller.dispose();
    _focusNode.dispose();
    _isScrollUIActive.dispose();
    _showNewMessageButton.dispose();
    SampChatController.channel.setMethodCallHandler(null);
    super.dispose();
  }

  void setVisible(bool visible) {
    if (!visible) hideInput();
    setState(() => _visible = visible);
  }

  void _updateHitRegion(Rect area) {
    if (_hitRegion == area) return;
    _hitRegion = area;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted && _hitRegion == area) {
        unawaited(SampChatController.notify('hitRegion', {
          'x': area.left, 'y': area.top, 'width': area.width, 'height': area.height,
        }));
      }
    });
  }

  void updateChatPosition(double value) {
    setState(() {
      chatPosValue = value;
    });
  }

  void _clearInputController() {
    _controller.value = const TextEditingValue(
      text: '',
      selection: TextSelection.collapsed(offset: 0),
    );
  }

  void hideInput() {
    if (!chatInputstatus) return;
    setState(() {
      chatInputstatus = false;
      _showNewMessageButton.value = false;
    });
    _focusNode.unfocus();
    _scrollToBottom();
  }

  void showInput() {
    setState(() {
      chatInputstatus = true;
    });
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted && chatInputstatus) _focusNode.requestFocus();
    });
  }

  void _onHoldComplete() {
    setState(() {
      isHolding = false;
      chatstatus = true;
      _showNewMessageButton.value = false;
    });
    _borderController.reset();
  }

  void _startHold() {
    setState(() => isHolding = true);
    _borderController.forward(from: 0);
  }

  void _cancelHold() {
    setState(() => isHolding = false);
    _borderController.reset();
  }

  void addMessage(String text, int type) {
    final bool reallyAtBottom = !_scrollController.hasClients || _scrollController.offset <= 20;

    setState(() {
      final spans = _parseSampText(text);
      _messages.insert(0, _ChatMessage(DateTime.now(), type, spans));

      if (_messages.length > widget.maxMessages) _messages.removeLast();

      _showNewMessageButton.value = !reallyAtBottom;
    });

    if (reallyAtBottom) _scrollToBottom();
  }

  void _scrollToBottom() {
    _showNewMessageButton.value = false;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted && _scrollController.hasClients) {
        _scrollController.animateTo(0,
            duration: const Duration(milliseconds: 200), curve: Curves.easeOut);
      }
    });
  }

  String _formatTime(DateTime time) {
    final h = time.hour.toString().padLeft(2, '0');
    final m = time.minute.toString().padLeft(2, '0');
    final s = time.second.toString().padLeft(2, '0');
    return "$h:$m:$s";
  }

  static final _sampColorRegex = RegExp(r'\{([0-9A-Fa-f]{6})\}');

  List<TextSpan> _parseSampText(String text) {
    final matches = _sampColorRegex.allMatches(text);
    List<TextSpan> spans = [];
    int lastIndex = 0;
    Color currentColor = Colors.white;

    for (final match in matches) {
      if (match.start > lastIndex) {
        spans.add(TextSpan(
          text: text.substring(lastIndex, match.start),
          style: TextStyle(color: currentColor),
        ));
      }
      final colorHex = match.group(1)!;
      currentColor = Color(int.parse("0xFF$colorHex"));
      lastIndex = match.end;
    }
    if (lastIndex < text.length) {
      spans.add(TextSpan(
        text: text.substring(lastIndex),
        style: TextStyle(color: currentColor),
      ));
    }
    return spans;
  }

  @override
  Widget build(BuildContext context) {
    if (!_visible) {
      _updateHitRegion(Rect.zero);
      return const SizedBox.shrink();
    }
    // ดึงขนาดหน้าจอจริงทางกายภาพในหน่วยพิกัดแบบ logical (dp) เพื่อความเสถียร
    final double screenWidth = View.of(context).physicalSize.width / View.of(context).devicePixelRatio;
    final double screenHeight = View.of(context).physicalSize.height / View.of(context).devicePixelRatio;
    final bool isLandscape = screenWidth > screenHeight;

    // คืนค่าสูตรคำนวณสเกลแบบดั้งเดิมของโครงการสำหรับแนวตั้งและแนวนอน
    final double baseScale = (isLandscape ? screenWidth / 850 : screenWidth / 420).clamp(0.85, 1.3);
    const double chatScale = 0.8;
    final double scale = baseScale * chatScale;

    // ✅ Optimization: ดึง Font Style ออกมาเตรียมไว้ครั้งเดียว ไม่ต้องเรียกซ้ำในทุกบรรทัดแชท (ประหยัด CPU)
    final TextStyle baseTextStyle = const TextStyle(fontFamily: "NotoSans");

    // คืนค่าความกว้างของแชทดั้งเดิมของโครงการ (60% แนวนอน, 90% แนวตั้ง)
    final safe = MediaQuery.viewPaddingOf(context);
    final bottom = math.max(safe.bottom, MediaQuery.viewInsetsOf(context).bottom);
    final double chatWidth = math.min(isLandscape ? screenWidth * 0.6 : screenWidth * 0.9,
        screenWidth - safe.left - safe.right - 8);

    // คืนค่าระยะเริ่มขอบซ้ายดั้งเดิมของโครงการ (15% แนวนอน, 20dp แนวตั้ง)
    final double defaultLeft = isLandscape ? screenWidth * 0.15 : 20.0;
    final double percent = chatPosValue.clamp(0.0, 16.0) / 16.0;
    double leftPos = percent * defaultLeft;
    leftPos = leftPos.clamp(safe.left + 4, math.max(safe.left + 4, screenWidth - safe.right - chatWidth));
    final double top = safe.top + 4.0 * scale;
    final double inputHeight = chatInputstatus ? 40.0 * scale : 0;
    final double messageHeight = math.max(0.0, math.min(
      (isLandscape ? 175.0 : 190.0) * scale,
      screenHeight - top - bottom - inputHeight,
    ));
    if (chatWidth <= 0 || messageHeight <= 0) {
      _updateHitRegion(Rect.zero);
      return const SizedBox.shrink();
    }
    _updateHitRegion(Rect.fromLTWH(leftPos, top, chatstatus ? 48 : chatWidth,
      chatstatus ? 48 : messageHeight + inputHeight));

    return Stack(
      children: [
        if (chatstatus)
          Positioned(
            top: top,
            left: leftPos,
            child: IconButton(
              iconSize: 18.0 * scale,
              icon: const Icon(Icons.chat, color: Colors.white),
              onPressed: () {
                setState(() {
                  chatstatus = false;
                  _showNewMessageButton.value = false;
                });
              },
            ),
          ),

        if (!chatstatus)
          AnimatedPositioned(
            duration: const Duration(milliseconds: 200),
            top: top,
            left: leftPos,
            width: chatWidth,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                GestureDetector(
                  behavior: HitTestBehavior.opaque,
                  onTap: () {
                    if (chatInputstatus) {
                      hideInput();
                    } else {
                      showInput();
                    }
                  },
                  onLongPressStart: (_) => _startHold(),
                  onLongPressEnd: (_) => _cancelHold(),
                  child: Stack(
                    children: [
                      AnimatedContainer(
                        duration: const Duration(milliseconds: 300),
                        curve: Curves.easeOutCubic,
                        // คืนค่าระดับความสูงแชทตามอัตราส่วนแนวตั้ง/แนวนอน
                        height: messageHeight,
                        padding: EdgeInsets.all(6.0 * scale),
                        decoration: BoxDecoration(
                          color: chatInputstatus
                              ? const Color.fromARGB(80, 0, 0, 0)
                              : Colors.transparent,
                          borderRadius: BorderRadius.circular(6.0 * scale),
                        ),
                        child: MouseRegion(
                          onEnter: (_) => _isScrollUIActive.value = true,
                          onExit: (_) => _isScrollUIActive.value = false,
                          child: ValueListenableBuilder<bool>(
                            valueListenable: _isScrollUIActive,
                            builder: (context, isActive, child) {
                              final Color thumbColor = isActive
                                  ? Colors.white.withOpacity(0.85)
                                  : Colors.white.withOpacity(0.35);
                              return RawScrollbar(
                                controller: _scrollController,
                                thumbVisibility: chatInputstatus,
                                thickness: 3.5 * scale,
                                radius: Radius.zero,
                                interactive: true,
                                padding: EdgeInsets.zero,
                                thumbColor: thumbColor,
                                child: child!,
                              );
                            },
                            child: NotificationListener<ScrollNotification>(
                              onNotification: (notification) {
                                  if (notification is ScrollStartNotification) {
                                    _isScrollUIActive.value = true;
                                  } else if (notification is ScrollEndNotification) {
                                    _isScrollUIActive.value = false;
                                  }
                                  return false;
                                },
                              child: MediaQuery.removePadding(
                                context: context,
                                removeLeft: true,
                                removeRight: true,
                                removeTop: true,
                                removeBottom: true,
                                child: ListView.builder(
                                  controller: _scrollController,
                                  reverse: true,
                                  padding: EdgeInsets.symmetric(vertical: 4.0 * scale),
                                  cacheExtent: 150,
                                  addRepaintBoundaries: false,
                                  addAutomaticKeepAlives: false,
                                  physics: chatInputstatus ? const BouncingScrollPhysics() : const NeverScrollableScrollPhysics(),
                                  itemCount: _messages.length,
                                  itemBuilder: (context, index) => _buildMessageRow(_messages[index], scale, baseTextStyle),
                                ),
                              ),
                            ),
                          ),
                        ),
                      ),
                      if (isHolding)
                        Positioned.fill(
                          child: RepaintBoundary(
                            child: AnimatedBuilder(
                              animation: _borderController,
                              builder: (_, __) => CustomPaint(
                                painter: _ProgressBorderPainter(_borderController.value),
                              ),
                            ),
                          ),
                        ),

                      ValueListenableBuilder<bool>(
                        valueListenable: _showNewMessageButton,
                        builder: (context, show, child) {
                          return Positioned(
                            bottom: 10.0 * scale,
                            left: 0,
                            right: 0,
                            child: Center(
                              child: AnimatedOpacity(
                                opacity: show ? 1.0 : 0.0,
                                duration: const Duration(milliseconds: 250),
                                child: IgnorePointer(
                                  ignoring: !show,
                                  child: _buildNewMessageButton(scale),
                                ),
                              ),
                            ),
                          );
                        },
                      ),
                    ],
                  ),
                ),

                AnimatedContainer(
                  duration: const Duration(milliseconds: 200),
                  curve: Curves.easeInOut,
                  height: chatInputstatus ? 36.0 * scale : 0.0,
                  margin: EdgeInsets.only(top: chatInputstatus ? 4.0 * scale : 0.0),
                  child: ClipRect(
                    child: OverflowBox(
                      alignment: Alignment.topCenter,
                      maxHeight: 36.0 * scale,
                      child: AnimatedOpacity(
                        opacity: chatInputstatus ? 1.0 : 0.0,
                        duration: const Duration(milliseconds: 200),
                        child: IgnorePointer(
                           ignoring: !chatInputstatus,
                          child: _buildChatInput(chatWidth, scale),
                        ),
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),
      ],
    );
  }

  Widget _buildMessageRow(_ChatMessage msg, double scale, TextStyle baseStyle) {
    final List<InlineSpan> strokeSpans = [];
    final List<InlineSpan> fillSpans = [];

    if (widget.showTimestamp) {
      if (widget.timestampStyle == 0) {
        final badgeWidget = Container(
          height: 14 * scale,
          margin: EdgeInsets.only(right: 6 * scale),
          padding: EdgeInsets.symmetric(horizontal: 5 * scale),
          decoration: BoxDecoration(
            color: const Color.fromARGB(120, 0, 0, 0),
            borderRadius: BorderRadius.circular(3.0 * scale),
            boxShadow: [
              BoxShadow(
                color: Colors.black.withOpacity(0.4),
                blurRadius: 3 * scale,
                offset: Offset(0, 1 * scale),
              ),
            ],
          ),
          child: Center(
            widthFactor: 1.0,
            child: Text(
              _formatTime(msg.time),
              style: TextStyle(
                color: const Color.fromARGB(255, 220, 220, 220),
                fontSize: 8.5 * scale,
                fontWeight: FontWeight.w500,
                height: 1.1,
              ),
            ),
          ),
        );

        strokeSpans.add(WidgetSpan(
          alignment: PlaceholderAlignment.middle,
          child: Opacity(opacity: 0, child: badgeWidget),
        ));
        fillSpans.add(WidgetSpan(
          alignment: PlaceholderAlignment.middle,
          child: badgeWidget,
        ));
      } else if (widget.timestampStyle == 1) {
        final String timeStr = "[${_formatTime(msg.time)}] ";
        final Color timeColor = msg.spans.isNotEmpty
            ? (msg.spans.first.style?.color ?? Colors.white)
            : Colors.white;

        strokeSpans.add(TextSpan(text: timeStr));
        fillSpans.add(TextSpan(
          text: timeStr,
          style: TextStyle(color: timeColor),
        ));
      }
    }

    if (msg.chattype > 0) {
      final tagWidget = Padding(
        padding: EdgeInsets.only(right: 6 * scale),
        child: _buildTypeTag(msg.chattype, scale),
      );

      strokeSpans.add(WidgetSpan(
        alignment: PlaceholderAlignment.middle,
        child: Opacity(opacity: 0, child: tagWidget),
      ));
      fillSpans.add(WidgetSpan(
        alignment: PlaceholderAlignment.middle,
        child: tagWidget,
      ));
    }

    for (final span in msg.spans) {
      strokeSpans.add(TextSpan(text: span.text));
      fillSpans.add(TextSpan(
        text: span.text,
        style: TextStyle(color: span.style?.color ?? Colors.white),
      ));
    }

    return Padding(
      padding: EdgeInsets.symmetric(vertical: 1.5 * scale, horizontal: 8 * scale),
      child: Stack(
        children: [
          // Layer 1: Stroke Outline (เส้นขอบสีดำรอบตัวอักษร)
          RichText(
            textAlign: TextAlign.left,
            text: TextSpan(
              style: baseStyle.copyWith(
                fontSize: 10.5 * scale,
                height: 1.35,
                foreground: Paint()
                  ..style = PaintingStyle.stroke
                  ..strokeWidth = 2.2 * scale
                  ..strokeCap = StrokeCap.round
                  ..strokeJoin = StrokeJoin.round
                  ..color = Colors.black,
              ),
              children: strokeSpans,
            ),
          ),
          // Layer 2: Fill Text (ข้อความสีหลัก)
          RichText(
            textAlign: TextAlign.left,
            text: TextSpan(
              style: baseStyle.copyWith(
                fontSize: 10.5 * scale,
                height: 1.35,
                color: Colors.white,
              ),
              children: fillSpans,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTypeTag(int type, double scale) {
    final color = type == 1 ? const Color(0xFFFFA600) : const Color(0xFFF1494A);
    return Container(
      height: 14 * scale,
      padding: EdgeInsets.symmetric(horizontal: 4 * scale),
      decoration: BoxDecoration(
        color: color.withOpacity(0.3),
        border: Border.all(color: color.withOpacity(0.8), width: 1.0 * scale),
        borderRadius: BorderRadius.circular(3.0 * scale),
      ),
      child: Center(
        widthFactor: 1.0,
        child: Icon(
          type == 1 ? Icons.warning_rounded : Icons.error_rounded,
          color: type == 1 ? const Color(0xFFFFA600) : const Color(0xFFF1494A),
          size: 10 * scale,
        ),
      ),
    );
  }

  Widget _buildChatInput(double width, double scale) {
    return Container(
      height: 36.0 * scale,
      width: width,
      padding: EdgeInsets.symmetric(horizontal: 8.0 * scale, vertical: 4.0 * scale),
      decoration: BoxDecoration(
        color: const Color.fromARGB(150, 0, 0, 0),
        borderRadius: BorderRadius.circular(6.0 * scale),
        border: Border.all(color: Colors.white10),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          ValueListenableBuilder<TextEditingValue>(
            valueListenable: _controller,
            builder: (context, value, child) {
              final bool isNotEmpty = value.text.isNotEmpty;
              return GestureDetector(
                onTap: () {
                  if (isNotEmpty) {
                    _controller.clear();
                  }
                },
                child: AnimatedSwitcher(
                  duration: const Duration(milliseconds: 200),
                  child: Icon(
                    isNotEmpty ? Icons.close : Icons.chat_bubble_outline,
                    key: ValueKey(isNotEmpty),
                    color: Colors.white,
                    size: 14.0 * scale,
                  ),
                ),
              );
            },
          ),
          SizedBox(width: 8.0 * scale),

          Expanded(
            child: TextField(
              controller: _controller,
              focusNode: _focusNode,
              enabled: chatInputstatus,
              textInputAction: TextInputAction.send,
              onChanged: (value) {},
              enableInteractiveSelection: true,
              style: TextStyle(
                color: Colors.white,
                fontSize: 11.0 * scale,
                height: 1.2,
              ),
              textAlignVertical: TextAlignVertical.center,
              decoration: InputDecoration(
                hintText: "พิมพ์ข้อความหรือ /คำสั่ง",
                hintStyle: const TextStyle(color: Colors.white38),
                border: InputBorder.none,
                isDense: true,
                contentPadding: EdgeInsets.symmetric(vertical: 6.0 * scale),
              ),
              onSubmitted: (_) => _handleSend(),
            ),
          ),

          IconButton(
            iconSize: 18.0 * scale,
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(),
            icon: const Icon(Icons.keyboard_arrow_up, color: Colors.white70),
            onPressed: () {
              if (history.isEmpty) return;
              historyIndex++;
              if (historyIndex >= history.length) {
                historyIndex = history.length - 1;
              }
              final newText = history[historyIndex];
              _controller.value = TextEditingValue(
                text: newText,
                selection: TextSelection.collapsed(offset: newText.length),
              );
            },
          ),

          IconButton(
            iconSize: 18.0 * scale,
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(),
            icon: const Icon(Icons.keyboard_arrow_down, color: Colors.white70),
            onPressed: () {
              if (history.isEmpty) return;
              historyIndex--;
              if (historyIndex < 0) {
                historyIndex = -1;
                _clearInputController();
              } else {
                final newText = history[historyIndex];
                _controller.value = TextEditingValue(
                  text: newText,
                  selection: TextSelection.collapsed(offset: newText.length),
                );
              }
            },
          ),

          SizedBox(width: 4.0 * scale),

          Container(
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(6.0 * scale),
              gradient: const LinearGradient(
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
                colors: [
                  Color(0xFFFFE082),
                  Color(0xFFFFC107),
                  Color(0xFFFFA000),
                ],
              ),
              boxShadow: [
                BoxShadow(
                  color: Colors.black.withOpacity(0.3),
                  offset: Offset(1 * scale, 1 * scale),
                  blurRadius: 3 * scale,
                ),
              ],
            ),
            child: IconButton(
              iconSize: 16.0 * scale,
              padding: EdgeInsets.all(5.0 * scale),
              constraints: const BoxConstraints(),
              tooltip: "ส่ง",
              icon: const Icon(Icons.send_rounded, color: Colors.black87),
              onPressed: _handleSend,
            ),
          ),
        ],
      ),
    );
  }

  Future<void> _handleSend() async {
    final text = _controller.text.trim();

    if (text.isEmpty) {
      hideInput();
      _clearInputController();
      return;
    }

    if (_sending) return;
    _sending = true;
    final sent = await SampChatController.sendText(text);
    if (!mounted) return;
    _sending = false;
    if (!sent) {
      addMessage('{FFCC00}ส่งไม่ได้ ข้อความยาวเกินไปหรือแชทยังไม่พร้อม', 0);
      return;
    }

    setState(() {
      history.insert(0, text);
      if (history.length > 100) {
        history.removeLast();
      }
      historyIndex = -1;
    });

    hideInput();
    _clearInputController();
  }

  Widget _buildNewMessageButton(double scale) {
    return GestureDetector(
      onTap: _scrollToBottom,
      child: Container(
        padding: EdgeInsets.symmetric(horizontal: 14.0 * scale, vertical: 6.0 * scale),
        decoration: BoxDecoration(
          gradient: const LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [
              Color(0xFFFFE082),
              Color(0xFFFFC107),
              Color(0xFFFFA000),
            ],
          ),
          borderRadius: BorderRadius.circular(25.0 * scale),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.4),
              blurRadius: 8 * scale,
              offset: Offset(0, 3 * scale),
            ),
          ],
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              "ข้อความใหม่",
              style: const TextStyle(fontFamily: "NotoSans").copyWith(
                color: Colors.black87,
                fontSize: 10.0 * scale,
                fontWeight: FontWeight.bold,
              ),
            ),
            SizedBox(width: 4 * scale),
            Icon(
              Icons.arrow_downward_rounded,
              color: Colors.black87,
              size: 12 * scale,
            ),
          ],
        ),
      ),
    );
  }
}

class _ProgressBorderPainter extends CustomPainter {
  final double progress;
  _ProgressBorderPainter(this.progress);

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.white
      ..strokeWidth = 4
      ..style = PaintingStyle.stroke;

    final rect = Rect.fromLTWH(0, 0, size.width, size.height);
    final rrect = RRect.fromRectAndRadius(rect, const Radius.circular(6));
    final path = Path()..addRRect(rrect);
    final metric = path.computeMetrics().first;
    final length = metric.length;
    final startOffset = size.width / 2;
    final current = (startOffset + progress * length) % length;
    final drawLength = progress * length;

    const dashWidth = 8.0;
    const dashSpace = 6.0;
    double distance = 0;

    while (distance < drawLength) {
      final dashStart = (current - distance) % length;
      final dashEnd = dashStart - dashWidth;
      Path extract;
      if (dashEnd < 0) {
        extract = Path()
          ..addPath(metric.extractPath(0, dashStart), Offset.zero)
          ..addPath(metric.extractPath(length + dashEnd, length), Offset.zero);
      } else {
        extract = metric.extractPath(dashEnd, dashStart);
      }
      canvas.drawPath(extract, paint);
      distance += dashWidth + dashSpace;
    }
  }

  @override
  bool shouldRepaint(covariant _ProgressBorderPainter oldDelegate) => oldDelegate.progress != progress;
}

class _ChatMessage {
  final DateTime time;
  final int chattype;
  final List<TextSpan> spans;

  _ChatMessage(this.time, this.chattype, this.spans);
}

class SampChatController {
  static const MethodChannel channel = MethodChannel('flutter_chat');

  static Future<void> notify(String method, [Object? args]) async {
    try {
      await channel.invokeMethod<void>(method, args);
    } on MissingPluginException {
      // Allows Flutter-only previews and tests.
    }
  }

  static Future<bool> sendText(String text) async {
    try {
      return await channel.invokeMethod<bool>('sendText', {'text': text}) ?? false;
    } on PlatformException {
      return false;
    } on MissingPluginException {
      return false;
    }
  }

  static void init(SampChatState state) {
    channel.setMethodCallHandler((call) async {
      if (!state.mounted) return;
      switch (call.method) {
        case 'show':
          state.setVisible(true);
          break;
        case 'hide':
          state.setVisible(false);
          break;
        case 'addMessage':
          final args = call.arguments as Map<Object?, Object?>;
          final message = args['message'];
          if (message is String) {
            state.addMessage((args['color'] as String? ?? '') + message, args['type'] as int? ?? 0);
          }
          break;
        case 'ChatPosition':
          state.updateChatPosition((call.arguments as num).toDouble());
          break;
        case 'HideInputChat':
          state.hideInput();
          break;
      }
    });
  }
}
