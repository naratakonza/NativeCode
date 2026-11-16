import 'dart:math' as math;
import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

const _dialogFont = TextStyle(
  fontFamily: 'Sarabun',
  fontFamilyFallback: ['sans-serif'],
);

class DialogState {
  static final ValueNotifier<Map?> currentDialog = ValueNotifier(null);
  static final ValueNotifier<bool> visible = ValueNotifier(false);
}

class DialogBridge {
  static const MethodChannel channel = MethodChannel('flutter_dialog');

  static void init() {
    channel.setMethodCallHandler((call) async {
      if (call.method == "showDialog") {
        final data = Map<String, dynamic>.from(call.arguments);
        DialogState.currentDialog.value = data;
      } else if (call.method == 'hideDialog') {
        DialogState.currentDialog.value = null;
      } else if (call.method == 'setDialogVisible') {
        DialogState.visible.value = call.arguments == true;
      }
    });
    channel.invokeMethod<void>('ready');
  }

  static void sendResponse(Map data, int response, int listItem, String input) {
    channel.invokeMethod<bool>("dialogResponse", {
      "id": data['id'],
      "sequence": data['sequence'],
      "response": response,
      "listitem": listItem,
      "input": input,
    });
  }
}

final _sampColorRegex = RegExp(r'\{([0-9A-Fa-f]{6})\}');

List<TextSpan> parseSampText(String text, {Color? defaultColor}) {
  text = String.fromCharCodes(text.runes.map((unit) =>
      unit >= 0xD800 && unit <= 0xDFFF ? 0xFFFD : unit));
  final RegExp regex = _sampColorRegex;
  List<TextSpan> spans = [];
  Color currentColor = defaultColor ?? Colors.white;
  int lastIndex = 0;

  for (final match in regex.allMatches(text)) {
    if (match.start > lastIndex) {
      spans.add(TextSpan(
        text: text.substring(lastIndex, match.start),
        style: _dialogFont.copyWith(color: currentColor),
      ));
    }
    currentColor = Color(int.parse("0xFF${match.group(1)!}"));
    lastIndex = match.end;
  }

  if (lastIndex < text.length) {
    spans.add(TextSpan(
      text: text.substring(lastIndex),
      style: _dialogFont.copyWith(color: currentColor),
    ));
  }
  return spans;
}

class SampDialog extends StatefulWidget {
  final Map data;
  const SampDialog({super.key, required this.data});

  @override
  State<SampDialog> createState() => _SampDialogState();
}

class _SampDialogState extends State<SampDialog> with SingleTickerProviderStateMixin {
  int selectedIndex = -1;
  final TextEditingController controller = TextEditingController();

  static const Color sampBg = Color(0xED1c2122);
  // static const Color sampHeader = Color(0xFF2C2519);
  static const Color sampOrange = Color(0xFFFFB800);
  static const Color sampDivider = Color(0xFF333333);
  bool _obscurePassword = true;

  final ScrollController _scrollController = ScrollController();
  final ValueNotifier<bool> _canScrollDown = ValueNotifier(false);
  late AnimationController _bounceController;
  late Animation<double> _bounceAnimation;

  final Map<int, GlobalKey> _itemKeys = {};
  final FocusNode _focusNode = FocusNode();

  @override
  void initState() {
    super.initState();
    _initSelection();
    
    _scrollController.addListener(_scrollListener);
    
    // สร้าง AnimationController แต่ยังไม่ repeat — จะเริ่มเมื่อ _canScrollDown เป็น true เท่านั้น
    // ป้องกัน animation วิ่งตลอดเวลาแม้ไม่มี scroll indicator แสดงอยู่ (ลด GPU load)
    _bounceController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
    );
    
    _bounceAnimation = Tween<double>(begin: 0.0, end: 5.0).animate(
      CurvedAnimation(parent: _bounceController, curve: Curves.easeInOut),
    );

    _canScrollDown.addListener(_onScrollIndicatorChanged);

    WidgetsBinding.instance.addPostFrameCallback((_) {
      _checkScroll();
    });

    // ลงทะเบียนรับ keyboard events ที่ระดับ Service Layer โดยตรง
    // วิธีนี้ไม่ขึ้นกับ Focus หรือ IME connection บน Android
    // ทำงานได้ทันทีโดยไม่ต้องรอให้ผู้ใช้แตะหน้าจอหรือเปิดคีย์บอร์ดมือถือก่อน
    HardwareKeyboard.instance.addHandler(_hardwareKeyHandler);
  }

  // เริ่ม/หยุด bounce animation ตามสถานะ scroll indicator จริงๆ
  // วิธีนี้ทำให้ไม่เปลือง GPU ตอนที่ไม่มีอะไรต้องแสดง
  void _onScrollIndicatorChanged() {
    if (_canScrollDown.value) {
      if (!_bounceController.isAnimating) {
        _bounceController.repeat(reverse: true);
      }
    } else {
      if (_bounceController.isAnimating) {
        _bounceController.stop();
      }
    }
  }

  bool _hardwareKeyHandler(KeyEvent event) {
    if (!DialogState.visible.value ||
        !identical(DialogState.currentDialog.value, widget.data)) return false;

    if (event is KeyDownEvent || event is KeyRepeatEvent) {
      if (event.logicalKey == LogicalKeyboardKey.arrowDown) {
        _handleArrow(1);
        return true;
      } else if (event.logicalKey == LogicalKeyboardKey.arrowUp) {
        _handleArrow(-1);
        return true;
      } else if (event.logicalKey == LogicalKeyboardKey.enter ||
                 event.logicalKey == LogicalKeyboardKey.numpadEnter) {
        _handleEnter();
        return true;
      } else if (event.logicalKey == LogicalKeyboardKey.escape) {
        _handleEscape();
        return true;
      }
    }
    return false;
  }

  void _checkScroll() {
    if (!mounted) return;
    if (_scrollController.hasClients) {
      // ตรวจสอบว่าสามารถเลื่อนลงไปได้อีกหรือไม่ (ให้ค่าเผื่อไว้ 2.0 pixels)
      final bool canScroll = _scrollController.position.maxScrollExtent - _scrollController.offset > 2.0;
      // อัปเดตผ่าน ValueNotifier — ไม่ต้อง setState() ทั้ง dialog
      // ValueListenableBuilder ใน _buildScrollIndicator จะ rebuild เฉพาะตัวเองเท่านั้น
      if (canScroll != _canScrollDown.value) {
        _canScrollDown.value = canScroll;
      }
    } else {
      if (_canScrollDown.value) {
        _canScrollDown.value = false;
      }
    }
  }

  void _scrollListener() {
    // เรียกตรง ไม่ผ่าน setState — scroll listener ไม่ควร trigger rebuild
    _checkScroll();
  }

  @override
  void didUpdateWidget(covariant SampDialog oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.data['id'] != oldWidget.data['id'] || widget.data['style'] != oldWidget.data['style']) {
      _initSelection();
      WidgetsBinding.instance.addPostFrameCallback((_) => _checkScroll());
    }
  }

  @override
  void dispose() {
    HardwareKeyboard.instance.removeHandler(_hardwareKeyHandler);
    _scrollController.removeListener(_scrollListener);
    _scrollController.dispose();
    _canScrollDown.removeListener(_onScrollIndicatorChanged);
    _canScrollDown.dispose();
    _bounceController.dispose();
    controller.dispose();
    _focusNode.dispose();
    super.dispose();
  }

  Widget _buildScrollIndicator(double scale) {
    return Positioned(
      bottom: 0,
      left: 0,
      right: 0,
      child: ValueListenableBuilder<bool>(
        valueListenable: _canScrollDown,
        builder: (context, canScroll, child) {
          // เมื่อ scroll ไม่ได้ ให้ซ่อน — ไม่ rebuild parent เพราะ Positioned ยังอยู่ใน Stack
          if (!canScroll) return const SizedBox.shrink();
          return child!;
        },
        child: IgnorePointer(
          child: Container(
            height: 35 * scale,
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
                colors: [
                  sampBg.withOpacity(0.0),
                  sampBg.withOpacity(0.95),
                ],
              ),
              borderRadius: BorderRadius.vertical(bottom: Radius.circular(3 * scale)),
            ),
            child: AnimatedBuilder(
              animation: _bounceAnimation,
              builder: (context, child) {
                return Transform.translate(
                  offset: Offset(0, -_bounceAnimation.value),
                  child: child,
                );
              },
              child: Align(
                alignment: Alignment.bottomCenter,
                child: Padding(
                  padding: EdgeInsets.only(bottom: 2 * scale),
                  child: Icon(
                    Icons.keyboard_arrow_down,
                    color: sampOrange,
                    size: 20 * scale,
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  void _handleArrow(int delta) {
    int style = widget.data['style'] ?? 0;
    if (style != 2 && style != 4 && style != 5) return;
    
    String contentStr = widget.data['content'] ?? "";
    List<String> lines = contentStr.split('\n').where((line) => line.trim().isNotEmpty).toList();
    if (lines.isEmpty) return;
    
    int minIndex = (style == 5) ? 1 : 0;
    int maxIndex = lines.length - 1;
    if (maxIndex < minIndex) return;

    int newIndex = selectedIndex + delta;
    if (newIndex < minIndex) newIndex = maxIndex;
    else if (newIndex > maxIndex) newIndex = minIndex;

    setState(() {
      selectedIndex = newIndex;
    });

    _scrollToSelected(newIndex, minIndex);
  }

  void _scrollToSelected(int newIndex, int minIndex) {
    final key = _itemKeys[newIndex];
    if (key != null && key.currentContext != null) {
      Scrollable.ensureVisible(
        key.currentContext!,
        duration: const Duration(milliseconds: 150),
        curve: Curves.easeInOut,
      );
    } else {
      if (!_scrollController.hasClients) return;
      final media = MediaQuery.of(context);
      final width = media.size.width - media.viewPadding.horizontal;
      final scale = (width / (media.orientation == Orientation.landscape
              ? 970.0 : 450.0)).clamp(0.65, 1.0);
      
      double itemHeight = 28.0 * scale; 
      int listIndex = newIndex - minIndex;
      double targetOffset = listIndex * itemHeight;
      
      _scrollController.animateTo(
        targetOffset.clamp(0.0, _scrollController.position.maxScrollExtent),
        duration: const Duration(milliseconds: 150),
        curve: Curves.easeInOut,
      );
    }
  }

  String _getInputText(int style) {
    if (style == 1 || style == 3) {
      return controller.text.trim();
    } else if (style == 2 || style == 4 || style == 5) {
      String contentStr = widget.data['content'] ?? "";
      List<String> lines = contentStr.split('\n').where((line) => line.trim().isNotEmpty).toList();
      if (selectedIndex >= 0 && selectedIndex < lines.length) {
        String line = lines[selectedIndex];
        List<String> cols = line.split('\t');
        if (cols.isNotEmpty) {
          return cols[0].replaceAll(_sampColorRegex, '').trim();
        }
      }
    }
    return "";
  }

  void _handleEnter() {
    int style = widget.data['style'] ?? 0;
    String inputText = _getInputText(style);
    int sendItem = (style == 5 && selectedIndex > 0) ? selectedIndex - 1 : selectedIndex;
    
    DialogBridge.sendResponse(widget.data, 1, sendItem, inputText);
  }

  void _handleEscape() {
    int style = widget.data['style'] ?? 0;
    String inputText = _getInputText(style);
    int sendItem = (style == 5 && selectedIndex > 0) ? selectedIndex - 1 : selectedIndex;
    
    DialogBridge.sendResponse(widget.data, 0, sendItem, inputText);
  }

  void _initSelection() {
    _itemKeys.clear();
    final int style = widget.data['style'] ?? 0;
    final String contentStr = widget.data['content'] ?? "";
    final List<String> lines = contentStr.split('\n').where((line) => line.trim().isNotEmpty).toList();

    if (style == 2 || style == 4) {
      if (lines.isNotEmpty && contentStr.isNotEmpty) {
        selectedIndex = 0;
      } else {
        selectedIndex = -1;
      }
    } else if (style == 5) {
      if (lines.length > 1) {
        selectedIndex = 1;
      } else {
        selectedIndex = -1;
      }
    } else {
      selectedIndex = -1;
    }
  }

  @override
  Widget build(BuildContext context) {
    final int style = widget.data['style'] ?? 0;
    final bool isCustomListStyle = (style == 2 || style == 4 || style == 5);

    final mediaQuery = MediaQuery.of(context);
    final safe = mediaQuery.viewPadding;
    final double screenWidth = math.max(0.0, mediaQuery.size.width - safe.horizontal);
    final double screenHeight = math.max(0.0, mediaQuery.size.height - safe.vertical);
    final bool isLandscape = mediaQuery.orientation == Orientation.landscape;
    final double bottomInset = mediaQuery.viewInsets.bottom;
    final double bottomPadding = math.max(safe.bottom, bottomInset);
    final double availableHeight = math.max(0.0, mediaQuery.size.height - safe.top - bottomPadding);

    double dialogWidth;
    if (isLandscape) {
      if (style == 4 || style == 5) {
        dialogWidth = (screenWidth * 0.50).clamp(340.0, 460.0);
      } else {
        dialogWidth = (screenWidth * 0.38).clamp(260.0, 340.0);
      }
    } else {
      dialogWidth = (screenWidth * 0.85).clamp(260.0, 380.0);
    }

    dialogWidth = math.min(dialogWidth, math.max(0.0, screenWidth - 24.0));

    double scale;
    if (isLandscape) {
      scale = (screenWidth / 970.0).clamp(0.65, 1.0);
    } else {
      scale = (screenWidth / 450.0).clamp(0.65, 1.0);
    }

    return GestureDetector(
      onTap: () {
        _focusNode.requestFocus();
      },
      behavior: HitTestBehavior.opaque,
      child: Focus(
        focusNode: _focusNode,
        autofocus: true,
        onKeyEvent: (FocusNode node, KeyEvent event) {
          if (event.logicalKey == LogicalKeyboardKey.arrowDown ||
              event.logicalKey == LogicalKeyboardKey.arrowUp ||
              event.logicalKey == LogicalKeyboardKey.enter ||
              event.logicalKey == LogicalKeyboardKey.numpadEnter ||
              event.logicalKey == LogicalKeyboardKey.escape) {
            return KeyEventResult.handled;
          }
          return KeyEventResult.ignored;
        },
        child: Padding(
          padding: EdgeInsets.fromLTRB(safe.left, safe.top, safe.right, bottomPadding),
          child: Center(
            child: Material(
              color: Colors.transparent,
              child: Container(
                width: dialogWidth,
                constraints: BoxConstraints(
                  maxHeight: math.max(0.0, math.min(availableHeight - 16.0 * scale, screenHeight * 0.85)),
                ),
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(12 * scale),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.65),
                      blurRadius: 28 * scale,
                      spreadRadius: 4 * scale,
                      offset: Offset(0, 10 * scale),
                    ),
                    BoxShadow(
                      color: const Color(0xFFFFB800).withOpacity(0.03),
                      blurRadius: 20 * scale,
                      spreadRadius: 1 * scale,
                    ),
                  ],
                ),
                child: ClipRRect(
                borderRadius: BorderRadius.circular(12 * scale),
                child: BackdropFilter(
                  filter: ImageFilter.blur(sigmaX: 12.0, sigmaY: 12.0),
                  child: Container(
                    decoration: BoxDecoration(
                      borderRadius: BorderRadius.circular(12 * scale),
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
                    child: SingleChildScrollView(
                      physics: bottomInset > 0 || style == 1 || style == 3
                          ? const BouncingScrollPhysics()
                          : const NeverScrollableScrollPhysics(),
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                        Container(
                          padding: EdgeInsets.fromLTRB(14 * scale, 12 * scale, 14 * scale, 10 * scale),
                          decoration: BoxDecoration(
                            color: Colors.transparent,
                            border: Border(
                              bottom: BorderSide(
                                color: const Color(0xFFFFB800).withOpacity(0.2),
                                width: 1.0,
                              ),
                            ),
                          ),
                          child: Row(
                            children: [
                              Container(
                                width: 5 * scale,
                                height: 5 * scale,
                                decoration: BoxDecoration(
                                  color: const Color(0xFFFFB800),
                                  shape: BoxShape.circle,
                                  boxShadow: [
                                    BoxShadow(
                                      color: const Color(0xFFFFB800).withOpacity(0.5),
                                      blurRadius: 4 * scale,
                                      spreadRadius: 1 * scale,
                                    ),
                                  ],
                                ),
                              ),
                              SizedBox(width: 8 * scale),
                              Expanded(
                                child: _text(
                                  widget.data['title']?.toString() ?? "",
                                  bold: true,
                                  color: Colors.white,
                                  size: 12.0 * scale,
                                ),
                              ),
                            ],
                          ),
                        ),
                        Container(
                          margin: EdgeInsets.fromLTRB(18 * scale, 12 * scale, 18 * scale, 4 * scale),
                          decoration: BoxDecoration(
                            color: (style == 0 || style == 1 || style == 3 || isCustomListStyle) 
                                ? Colors.transparent 
                                : const Color(0xFF222222),
                            borderRadius: BorderRadius.circular(3 * scale),
                            border: (style == 0 || style == 1 || style == 3 || isCustomListStyle) 
                                ? null 
                                : Border.all(color: sampDivider),
                          ),
                          child: _buildContent(scale, screenHeight, isLandscape),
                        ),
                        SizedBox(height: 10 * scale),
                        Padding(
                          padding: EdgeInsets.fromLTRB(18 * scale, 0, 18 * scale, 16 * scale),
                          child: _buildButtons(scale),
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
      ),
    );
  }

  Widget _buildContent(double scale, double screenHeight, bool isLandscape) {
    int style = widget.data['style'] ?? 0;
    String contentStr = widget.data['content'] ?? "";

    if (style == 1 || style == 3) {
      return Padding(
        padding: EdgeInsets.symmetric(horizontal: 0, vertical: 6.0 * scale),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _text(contentStr, size: 10.0 * scale),
            SizedBox(height: 8 * scale),
            Stack(
              alignment: Alignment.centerRight,
              children: [
                TextField(
                  controller: controller,
                  obscureText: (style == 3 && _obscurePassword),
                  obscuringCharacter: '•',
                  maxLines: 1,
                  style: _dialogFont.copyWith(
                    color: Colors.white, 
                    fontSize: 10.5 * scale,
                  ),
                  cursorColor: sampOrange,
                  cursorHeight: 12 * scale,
                  decoration: InputDecoration(
                    hintText: 'กรอกข้อมูลที่นี่...',
                    hintStyle: _dialogFont.copyWith(
                      color: Colors.white.withOpacity(0.25), 
                      fontSize: 10.5 * scale,
                    ),
                    filled: true, 
                    fillColor: Colors.black.withOpacity(0.2),
                    contentPadding: EdgeInsets.fromLTRB(10 * scale, 8 * scale, 28 * scale, 8 * scale),
                    isDense: true, 
                    enabledBorder: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(8 * scale),
                      borderSide: BorderSide(color: Colors.white.withOpacity(0.08), width: 1.0),
                    ),
                    focusedBorder: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(8 * scale),
                      borderSide: const BorderSide(color: Color(0xFFFFB800), width: 1.2),
                    ),
                  ),
                ),
                Padding(
                  padding: EdgeInsets.only(right: 6 * scale),
                  child: _buildSuffixIcon(style, scale),
                ),
              ],
            ),
          ],
        ),
      );
    }

    if (style == 0) {
      final double maxMsgBoxHeight = isLandscape 
          ? (screenHeight * 0.48).clamp(100.0, 300.0) 
          : (screenHeight * 0.58).clamp(120.0, 450.0);

      return Padding(
        padding: EdgeInsets.symmetric(vertical: 4.0 * scale),
        child: Container(
          constraints: BoxConstraints(
            maxHeight: maxMsgBoxHeight,
          ),
          child: Stack(
            children: [
              SingleChildScrollView(
                controller: _scrollController,
                physics: const BouncingScrollPhysics(),
                child: Container(
                  width: double.infinity,
                  color: Colors.transparent,
                  child: _text(contentStr, size: 10.0 * scale),
                ),
              ),
              _buildScrollIndicator(scale),
            ],
          ),
        ),
      );
    }

    List<String> lines = contentStr.split('\n').where((line) => line.trim().isNotEmpty).toList();

    int maxCols = 0;
    List<int> columnMaxLengths = [];
    if (style == 4 || style == 5) {
      for (var line in lines) {
        int colsCount = line.split('\t').length;
        if (colsCount > maxCols) {
          maxCols = colsCount;
        }
      }
      columnMaxLengths = List<int>.filled(maxCols, 0);
      for (var line in lines) {
        List<String> cols = line.split('\t');
        for (int i = 0; i < maxCols; i++) {
          String colText = i < cols.length ? cols[i] : "";
          int len = colText.replaceAll(_sampColorRegex, '').length;
          if (len > columnMaxLengths[i]) {
            columnMaxLengths[i] = len;
          }
        }
      }
    }

    double itemEstimatedHeight = 28.0 * scale;
    
    // ปรับสัดส่วนความสูงของรายการลิสต์ เพื่อความปลอดภัยในการเลื่อนดู
    final double maxListHeightLimit = isLandscape 
        ? (screenHeight * 0.48).clamp(100.0, 300.0) 
        : (screenHeight * 0.58).clamp(120.0, 450.0);
        
    double maxListHeight = (itemEstimatedHeight * (style == 5 ? 9 : 8)).clamp(80.0, maxListHeightLimit); 

    return Column(
      mainAxisSize: MainAxisSize.min,
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        if (style == 5 && lines.isNotEmpty)
          _renderRow(lines[0].split('\t'), 0, true, scale, maxCols: maxCols, columnMaxLengths: columnMaxLengths),
        Flexible(
          child: Container(
            constraints: BoxConstraints(
              maxHeight: (style == 5 && lines.isNotEmpty) ? maxListHeight - itemEstimatedHeight : maxListHeight
            ),
            child: Stack(
              children: [
                ListView.builder(
                  controller: _scrollController,
                  shrinkWrap: true,
                  itemCount: (style == 5 && lines.isNotEmpty) ? lines.length - 1 : lines.length,
                  padding: EdgeInsets.zero,
                  physics: const BouncingScrollPhysics(),
                  itemBuilder: (context, index) {
                    int actualIndex = (style == 5 && lines.isNotEmpty) ? index + 1 : index;
                    if (style == 2) {
                      return _renderRow([lines[actualIndex]], actualIndex, false, scale, isStyle2: true);
                    } else {
                      return _renderRow(lines[actualIndex].split('\t'), actualIndex, style == 5, scale, maxCols: maxCols, columnMaxLengths: columnMaxLengths);
                    }
                  },
                ),
                _buildScrollIndicator(scale),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget? _buildSuffixIcon(int style, double scale) {
    if (style == 1) {
      if (controller.text.isEmpty) return const SizedBox();
      return GestureDetector(
        onTap: () {
          setState(() {
            controller.clear();
          });
        },
        behavior: HitTestBehavior.opaque,
        child: Container(
          padding: EdgeInsets.all(5 * scale),
          child: Icon(Icons.close, color: Colors.white.withOpacity(0.3), size: 11 * scale),
        ),
      );
    }
    
    if (style == 3) {
      return GestureDetector(
        onTap: () {
          setState(() {
            _obscurePassword = !_obscurePassword;
          });
        },
        behavior: HitTestBehavior.opaque,
        child: Container(
          padding: EdgeInsets.all(5 * scale),
          child: Icon(
            _obscurePassword ? Icons.visibility_off : Icons.visibility,
            color: _obscurePassword ? Colors.white.withOpacity(0.4) : sampOrange,
            size: 11 * scale,
          ),
        ),
      );
    }
    return const SizedBox();
  }

  Widget _renderRow(List<String> cols, int index, bool hasHeader, double scale, {bool isStyle2 = false, int maxCols = 0, List<int>? columnMaxLengths}) {
    bool isHeader = hasHeader && index == 0;
    bool selected = selectedIndex == index;

    _itemKeys[index] ??= GlobalKey();

    Color rowBgColor = isHeader
        ? Colors.white.withOpacity(0.06)
        : (selected ? const Color(0xFFFFB800).withOpacity(0.12) : Colors.white.withOpacity(0.025));

    Color borderColor = isHeader 
        ? Colors.transparent 
        : (selected ? const Color(0xFFFFB800).withOpacity(0.35) : Colors.transparent);

    List<String> paddedCols = List<String>.from(cols);
    if (paddedCols.length < maxCols) {
      paddedCols.addAll(List<String>.filled(maxCols - paddedCols.length, ""));
    }

    return Padding(
      key: _itemKeys[index],
      padding: EdgeInsets.symmetric(vertical: 2.0 * scale),
      child: InkWell(
        highlightColor: const Color(0xFFFFB800).withOpacity(0.1),
        splashColor: const Color(0xFFFFB800).withOpacity(0.2),
        onTap: isHeader ? null : () => setState(() => selectedIndex = index),
        onDoubleTap: isHeader ? null : () {
          setState(() => selectedIndex = index);
          int style = widget.data['style'] ?? 0;
          String inputText = _getInputText(style);
          int sendItem = (style == 5 && index > 0) ? index - 1 : index;
          DialogBridge.sendResponse(widget.data, 1, sendItem, inputText);
        },
        borderRadius: BorderRadius.circular(4 * scale),
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 200),
          curve: Curves.easeOutCubic,
          clipBehavior: Clip.hardEdge,
          decoration: BoxDecoration(
            color: rowBgColor,
            borderRadius: BorderRadius.circular(4 * scale),
            border: isHeader ? null : Border.all(color: borderColor, width: 1.0),
          ),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Stack(
                children: [
                  Padding(
                    padding: EdgeInsets.symmetric(
                      vertical: (isHeader ? 9.0 : 8.5) * scale,
                      horizontal: 12 * scale,
                    ),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.center,
                      children: List.generate(paddedCols.length, (colIndex) {
                        final col = paddedCols[colIndex];
                        bool shouldExpand = selected && (widget.data['style'] == 4 || widget.data['style'] == 5);
                        
                        int flex = 1;
                        if (columnMaxLengths != null && colIndex < columnMaxLengths.length) {
                          flex = columnMaxLengths[colIndex].clamp(3, 30);
                        }

                        return Expanded(
                          flex: flex,
                          child: Align(
                            alignment: isStyle2 ? Alignment.center : Alignment.centerLeft,
                            child: Padding(
                              padding: EdgeInsets.only(right: 4.0 * scale),
                              child: _text(
                                col,
                                bold: isHeader || selected,
                                color: (isStyle2 && selected) ? const Color(0xFFFFB800) : Colors.white,
                                size: 11.0 * scale,
                                maxLines: shouldExpand ? null : 1,
                                overflow: shouldExpand ? TextOverflow.visible : TextOverflow.ellipsis,
                              ),
                            ),
                          ),
                        );
                      }),
                    ),
                  ),
                  if (!isHeader)
                    Positioned(
                      left: 0,
                      top: 0,
                      bottom: 0,
                      child: Center(
                        child: AnimatedScale(
                          scale: selected ? 1.0 : 0.0,
                          duration: const Duration(milliseconds: 200),
                          curve: Curves.easeInOut,
                          child: Container(
                            width: 3.0 * scale,
                            height: 16 * scale,
                            decoration: BoxDecoration(
                              color: const Color(0xFFFFB800),
                              borderRadius: BorderRadius.only(
                                topRight: Radius.circular(2 * scale),
                                bottomRight: Radius.circular(2 * scale),
                              ), 
                            ),
                          ),
                        ),
                      ),
                    ),
                ],
              ),
              if (isHeader)
                Container(
                  height: 1.5 * scale,
                  width: double.infinity,
                  decoration: BoxDecoration(
                    color: sampOrange,
                    borderRadius: BorderRadius.vertical(bottom: Radius.circular(4 * scale)),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildButtons(double scale) {
    int style = widget.data['style'] ?? 0;
    String inputText = _getInputText(style);
    int sendItem = (style == 5 && selectedIndex > 0) ? selectedIndex - 1 : selectedIndex;

    return Row(
      children: [
        Expanded(
          child: _btn(widget.data['button1']?.toString() ?? "", scale, () {
            DialogBridge.sendResponse(widget.data, 1, sendItem, inputText);
          }),
        ),
        if (widget.data['button2'] != null && widget.data['button2'] != "") ...[
          SizedBox(width: 8 * scale),
          Expanded(
            child: _btn(widget.data['button2']?.toString() ?? "", scale, () {
              DialogBridge.sendResponse(widget.data, 0, sendItem, inputText);
            },
            isButton2: true,
            ),
          ),
        ]
      ],
    );
  }

  Widget _text(String text, {bool bold = false, Color? color, double size = 14, TextAlign align = TextAlign.left, int? maxLines, TextOverflow overflow = TextOverflow.clip}) {
    return RichText(
      textAlign: align,
      maxLines: maxLines,
      overflow: overflow,
      text: TextSpan(
        children: parseSampText(text, defaultColor: color),
        style: _dialogFont.copyWith(
          color: color ?? Colors.white,
          fontWeight: bold ? FontWeight.bold : FontWeight.normal,
          fontSize: size,
        ),
      ),
    );
  }

  Widget _btn(String text, double scale, VoidCallback onTap, {bool isButton2 = false}) {
    return Container(
      height: 36 * scale,
      decoration: BoxDecoration(
        color: isButton2 ? Colors.white.withOpacity(0.06) : const Color(0xFFFFB800),
        borderRadius: BorderRadius.circular(10 * scale),
        border: isButton2 
            ? Border.all(color: Colors.white.withOpacity(0.12), width: 1.0)
            : null,
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          borderRadius: BorderRadius.circular(10 * scale),
          onTap: onTap,
          splashColor: isButton2 ? Colors.white.withOpacity(0.08) : Colors.black.withOpacity(0.15),
          highlightColor: isButton2 ? Colors.white.withOpacity(0.04) : Colors.black.withOpacity(0.08),
          child: Align(
            alignment: Alignment.center,
            child: Padding(
              padding: EdgeInsets.symmetric(horizontal: 4 * scale),
              child: _text(
                text,
                bold: true,
                color: isButton2 ? Colors.white.withOpacity(0.85) : Colors.black,
                size: 11.5 * scale,
                align: TextAlign.center,
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class SampDialogOverlay extends StatelessWidget {
  const SampDialogOverlay({super.key});

  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder<Map?>(
      valueListenable: DialogState.currentDialog,
      builder: (context, data, _) {
        if (data == null) return const SizedBox();
        // RepaintBoundary แยก dialog ออกจาก game surface ของ Flutter
        // โดยไม่มี RepaintBoundary: ทุก frame ที่ dialog rebuild จะทำให้ game layer
        // ต้อง repaint ด้วย → FPS drop 10-20fps ทันทีที่ dialog โผล่
        // ด้วย RepaintBoundary: dialog มี compositing layer ของตัวเอง
        // game ไม่ต้อง repaint เมื่อ dialog เปลี่ยนแปลง
        return ValueListenableBuilder<bool>(
          valueListenable: DialogState.visible,
          builder: (context, visible, child) => Visibility(
            visible: visible,
            maintainState: true,
            child: ExcludeFocus(excluding: !visible, child: child!),
          ),
          child: RepaintBoundary(
            child: Container(
              color: Colors.black.withOpacity(0.4),
              child: SampDialog(key: ValueKey(data['sequence']), data: data),
            ),
          ),
        );
      },
    );
  }
}
