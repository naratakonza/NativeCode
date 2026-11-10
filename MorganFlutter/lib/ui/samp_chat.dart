import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class SampChat extends StatefulWidget {
  const SampChat({super.key, this.maxMessages = 40});

  final int maxMessages;

  @override
  State<SampChat> createState() => SampChatState();
}

class SampChatState extends State<SampChat> {
  final List<_ChatMessage> _messages = [];
  final List<String> _history = [];
  final ScrollController _scrollController = ScrollController();
  final TextEditingController _controller = TextEditingController();
  final FocusNode _focusNode = FocusNode();
  static final _sampColorRegex = RegExp(r'\{([0-9A-Fa-f]{6})\}');
  bool _visible = false;
  bool _inputVisible = false;
  bool _collapsed = false;
  bool _sending = false;
  String? _sendError;
  int _historyIndex = 0;
  Rect? _hitRegion;

  @override
  void initState() {
    super.initState();
    SampChatController.channel.setMethodCallHandler(_onNativeMessage);
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) unawaited(SampChatController.notify('ready'));
    });
  }

  @override
  void dispose() {
    SampChatController.channel.setMethodCallHandler(null);
    _scrollController.dispose();
    _controller.dispose();
    _focusNode.dispose();
    super.dispose();
  }

  Future<void> _onNativeMessage(MethodCall call) async {
    switch (call.method) {
      case 'show':
        setState(() => _visible = true);
        break;
      case 'hide':
        hideInput();
        setState(() => _visible = false);
        break;
      case 'HideInputChat':
        hideInput();
        break;
      case 'addMessage':
        final args = call.arguments as Map<Object?, Object?>;
        final text = args['message'];
        if (text is String) {
          addMessage('${args['color'] ?? ''}$text', args['type'] as int? ?? 0);
        }
        break;
    }
  }

  void addMessage(String text, int type) {
    setState(() {
      _messages.insert(0, _ChatMessage(_parseSampText(text), DateTime.now(), type));
      if (_messages.length > widget.maxMessages) _messages.removeLast();
    });
  }

  List<TextSpan> _parseSampText(String text) {
    final spans = <TextSpan>[];
    var start = 0;
    var color = Colors.white;
    for (final match in _sampColorRegex.allMatches(text)) {
      if (match.start > start) {
        spans.add(TextSpan(
          text: text.substring(start, match.start),
          style: TextStyle(color: color),
        ));
      }
      color = Color(0xff000000 | int.parse(match.group(1)!, radix: 16));
      start = match.end;
    }
    spans.add(TextSpan(
      text: text.substring(start),
      style: TextStyle(color: color),
    ));
    return spans;
  }

  void showInput() {
    setState(() {
      _collapsed = false;
      _inputVisible = true;
      _sendError = null;
      _historyIndex = _history.length;
    });
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted && _inputVisible) _focusNode.requestFocus();
    });
  }

  void hideInput() {
    if (!_inputVisible) return;
    _focusNode.unfocus();
    setState(() => _inputVisible = false);
    if (_scrollController.hasClients) _scrollController.jumpTo(0);
  }

  void _moveHistory(int offset) {
    if (_history.isEmpty) return;
    _historyIndex = (_historyIndex + offset).clamp(0, _history.length).toInt();
    final text = _historyIndex == _history.length ? '' : _history[_historyIndex];
    _controller.value = TextEditingValue(
      text: text,
      selection: TextSelection.collapsed(offset: text.length),
    );
  }

  Future<void> _send() async {
    final text = _controller.text.trim();
    if (text.isEmpty || _sending) return;
    setState(() => _sending = true);
    final accepted = await SampChatController.sendText(text);
    if (!mounted) return;
    setState(() {
      _sending = false;
      _sendError = accepted ? null : 'ส่งไม่ได้ ข้อความยาวเกินไปหรือแชทยังไม่พร้อม';
    });
    if (accepted) {
      if (_history.isEmpty || _history.last != text) _history.add(text);
      if (_history.length > widget.maxMessages) _history.removeAt(0);
      _controller.clear();
      hideInput();
    }
  }

  void _updateHitRegion(Rect rect) {
    if (_hitRegion == rect) return;
    _hitRegion = rect;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted && _hitRegion == rect) {
        unawaited(SampChatController.notify('hitRegion', {
          'x': rect.left,
          'y': rect.top,
          'width': rect.width,
          'height': rect.height,
        }));
      }
    });
  }

  String _formatTime(DateTime time) =>
      [time.hour, time.minute, time.second].map((n) => '$n'.padLeft(2, '0')).join(':');

  @override
  Widget build(BuildContext context) {
    if (!_visible) {
      _updateHitRegion(Rect.zero);
      return const SizedBox.shrink();
    }

    return LayoutBuilder(builder: (context, constraints) {
      final screenWidth = constraints.maxWidth;
      final screenHeight = constraints.maxHeight;
      final landscape = screenWidth > screenHeight;
      final scale = (screenWidth / (landscape ? 850 : 420)).clamp(0.85, 1.3).toDouble() * 0.8;
      final safe = MediaQuery.viewPaddingOf(context);
      final keyboard = MediaQuery.viewInsetsOf(context).bottom;
      final left = math.max(safe.left + 4, landscape ? screenWidth * 0.15 : 20.0);
      final top = safe.top + 4;
      final width = math.min(screenWidth * (landscape ? 0.6 : 0.9), screenWidth - left - safe.right - 4);
      final availableHeight = math.max(0.0, screenHeight - math.max(safe.bottom, keyboard) - top);
      final height = math.min(availableHeight, (landscape ? 175.0 : 190.0) * scale + (_inputVisible ? 64 : 0));
      final buttonHeight = math.min(44.0, height);
      if (width < 44 || height <= 0) {
        _updateHitRegion(Rect.zero);
        return const SizedBox.shrink();
      }
      _updateHitRegion(Rect.fromLTWH(left, top, _inputVisible ? width : 44, _inputVisible ? height : buttonHeight));

      return Stack(children: [
        Positioned(
          top: top,
          left: left,
          width: width,
          height: height,
          child: Material(
            color: _inputVisible ? Colors.black54 : Colors.transparent,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                SizedBox(
                  height: buttonHeight,
                  width: 44,
                  child: IconButton(
                    tooltip: _inputVisible ? 'ปิดแชท' : 'เปิดแชท',
                    color: Colors.white,
                    padding: EdgeInsets.zero,
                    onPressed: _inputVisible ? hideInput : showInput,
                    icon: GestureDetector(
                      onLongPress: _inputVisible ? null : () => setState(() => _collapsed = !_collapsed),
                      child: Icon(_inputVisible ? Icons.close : Icons.chat),
                    ),
                  ),
                ),
                if (!_collapsed)
                  Expanded(
                    child: ListView.builder(
                      controller: _scrollController,
                      reverse: true,
                      padding: const EdgeInsets.symmetric(horizontal: 5),
                      physics: _inputVisible ? const BouncingScrollPhysics() : const NeverScrollableScrollPhysics(),
                      itemCount: _messages.length,
                      itemBuilder: (context, index) {
                        final message = _messages[index];
                        return Text.rich(
                          TextSpan(children: [
                            TextSpan(text: '[${_formatTime(message.time)}] ', style: const TextStyle(color: Colors.white70)),
                            ...message.spans,
                          ]),
                          style: TextStyle(
                            color: Colors.white,
                            fontSize: 16 * scale,
                            fontWeight: message.type == 1 ? FontWeight.bold : FontWeight.normal,
                            shadows: const [Shadow(blurRadius: 3, color: Colors.black)],
                          ),
                        );
                      },
                    ),
                  ),
                if (_inputVisible && height >= 90)
                  Row(children: [
                    IconButton(
                      tooltip: 'ข้อความก่อนหน้า',
                      color: Colors.white,
                      onPressed: () => _moveHistory(-1),
                      icon: const Icon(Icons.keyboard_arrow_up),
                    ),
                    IconButton(
                      tooltip: 'ข้อความถัดไป',
                      color: Colors.white,
                      onPressed: () => _moveHistory(1),
                      icon: const Icon(Icons.keyboard_arrow_down),
                    ),
                    Expanded(
                      child: TextField(
                        controller: _controller,
                        focusNode: _focusNode,
                        style: const TextStyle(color: Colors.white),
                        textInputAction: TextInputAction.send,
                        onSubmitted: (_) => unawaited(_send()),
                        decoration: const InputDecoration(
                          isDense: true,
                          hintText: 'พิมพ์ข้อความหรือ /คำสั่ง',
                          hintStyle: TextStyle(color: Colors.white70),
                          border: InputBorder.none,
                        ),
                      ),
                    ),
                    IconButton(
                      tooltip: 'ส่ง',
                      color: Colors.white,
                      onPressed: _sending ? null : () => unawaited(_send()),
                      icon: const Icon(Icons.send),
                    ),
                  ]),
                if (_inputVisible && height >= 110 && _sendError != null)
                  Text(_sendError!, maxLines: 1, style: const TextStyle(color: Colors.amber, fontSize: 12)),
              ],
            ),
          ),
        ),
      ]);
    });
  }
}

class _ChatMessage {
  const _ChatMessage(this.spans, this.time, this.type);

  final List<TextSpan> spans;
  final DateTime time;
  final int type;
}

class SampChatController {
  static const channel = MethodChannel('flutter_chat');

  static Future<void> notify(String method, [Object? arguments]) async {
    try {
      await channel.invokeMethod<void>(method, arguments);
    } on MissingPluginException {
      // The Flutter widget can also run on its own.
    }
  }

  static Future<bool> sendText(String text) async {
    try {
      return await channel.invokeMethod<bool>('sendText', {'text': text}) ?? false;
    } on MissingPluginException {
      return false;
    } on PlatformException {
      return false;
    }
  }
}
