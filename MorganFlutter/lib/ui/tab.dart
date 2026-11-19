import 'dart:ui';
import 'dart:math' as math;
import 'scoreboard_state.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

const _scoreBoardFont = TextStyle(
  fontFamily: 'Sarabun',
  fontFamilyFallback: ['sans-serif'],
);

class PlayerData {
  int id;
  String name;
  int score;
  int ping;
  Color color;

  PlayerData({
    required this.id,
    required this.name,
    required this.score,
    required this.ping,
    required this.color,
  });
}

class ScoreBoardBridge {
  static const MethodChannel channel = MethodChannel('flutter_scoreboard');

  static void init() {
    channel.setMethodCallHandler((call) async {
      switch (call.method) {
        case 'setScoreBoardVisible':
          ScoreBoardState.visible.value = call.arguments == true;
          break;
        case "updateplayer":
          final List<dynamic> players = call.arguments;
          final Set<int> receivedIds = {};

          for (final p in players) {
            final id = p[0] as int;
            receivedIds.add(id);

            UIFTabScreenState.instance?.updatePlayerDataOnly(
              id,
              p[1] as String,
              p[2] as int,
              p[3] as int,
              p[4] as int,
            );
          }
          UIFTabScreenState.instance?.syncPlayers(receivedIds);
          if (ScoreBoardState.visible.value) {
            UIFTabScreenState.instance?.selectFirstIfNone();
            UIFTabScreenState.instance?.refreshUI();
            print("Update Player Flutter");
          }
          break;

        case "serverinfo":
          final List<dynamic> data = call.arguments;

          UIFTabScreenState.instance?.serverName.value = data[0] as String;

          UIFTabScreenState.instance?.playerOnline.value = data[1] as int;
          break;

        case "localplayerid":
          final int localId = call.arguments as int;
          if (UIFTabScreenState.instance != null) {
            UIFTabScreenState.instance!.localPlayerId = localId;
            UIFTabScreenState.instance!.selectFirstIfNone();
            UIFTabScreenState.instance!.refreshUI();
          }
          break;
      }
      return null;
    });
  }

  static Future<void> Tabclose() async {
    try {
      await channel.invokeMethod("TabClose");
    } on PlatformException catch (e) {
      print("Failed: '${e.message}'");
    }
  }

  static Future<void> sendTab(int playerid) async {
    try {
      await channel.invokeMethod('onPlayerTab', {"playerid": playerid});
    } on PlatformException catch (e) {
      print("Failed: '${e.message}'");
    }
  }
}

class UIFTabScreen extends StatefulWidget {
  final bool visible;
  const UIFTabScreen({super.key, this.visible = false});

  @override
  State<UIFTabScreen> createState() => UIFTabScreenState();
}

class UIFTabScreenState extends State<UIFTabScreen>
    with SingleTickerProviderStateMixin {
  static UIFTabScreenState? instance;
  final Map<int, PlayerData> players = {};
  final TextEditingController searchController = TextEditingController();
  final FocusNode searchFocusNode = FocusNode();
  final ValueNotifier<String> serverName = ValueNotifier("");
  final ValueNotifier<int> playerOnline = ValueNotifier(0);
  String searchText = "";
  final ValueNotifier<int> hoveredIdNotifier = ValueNotifier(-1);
  final ValueNotifier<int> selectedIdNotifier = ValueNotifier(-1);
  int localPlayerId = -1;
  late double scale;

  static final RegExp _sampColorRegex = RegExp(r'\{([0-9A-Fa-f]{6})\}');

  String _sanitizeUtf16(String input) {
    if (input.isEmpty) return input;
    final codeUnits = input.codeUnits;
    final length = codeUnits.length;
    final List<int> cleanUnits = [];

    for (int i = 0; i < length; i++) {
      final int unit = codeUnits[i];
      if (unit >= 0xD800 && unit <= 0xDBFF) {
        if (i + 1 < length) {
          final int next = codeUnits[i + 1];
          if (next >= 0xDC00 && next <= 0xDFFF) {
            cleanUnits.add(unit);
            cleanUnits.add(next);
            i++;
            continue;
          }
        }
        cleanUnits.add(0xFFFD);
      } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
        cleanUnits.add(0xFFFD);
      } else {
        cleanUnits.add(unit);
      }
    }

    return String.fromCharCodes(cleanUnits);
  }

  List<TextSpan> _parseSampText(String text) {
    text = _sanitizeUtf16(text);
    final matches = _sampColorRegex.allMatches(text);
    List<TextSpan> spans = [];
    int lastIndex = 0;
    Color currentColor = Colors.white; // default color in top bar is white

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

  late AnimationController _animationController;
  late Animation<double> _scaleAnimation;
  late Animation<double> _opacityAnimation;
  late Animation<Offset> _slideAnimation;
  late Animation<double> _blurAnimation;

  double rs(double size) => size * scale;

  @override
  void initState() {
    super.initState();
    instance = this;

    _animationController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 350),
    );

    _scaleAnimation = Tween<double>(begin: 0.85, end: 1.0).animate(
      CurvedAnimation(
        parent: _animationController,
        curve: const Cubic(
            0.175, 0.885, 0.32, 1.275), // Custom spring easeOutBack curve
        reverseCurve: Curves.easeIn,
      ),
    );

    _opacityAnimation = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(
        parent: _animationController,
        curve: Curves.easeOut,
        reverseCurve: Curves.easeIn,
      ),
    );

    _slideAnimation = Tween<Offset>(
      begin: const Offset(0.0, 0.05),
      end: Offset.zero,
    ).animate(
      CurvedAnimation(
        parent: _animationController,
        curve: const Cubic(0.25, 1.0, 0.5, 1.0),
        reverseCurve: Curves.easeIn,
      ),
    );

    _blurAnimation = Tween<double>(begin: 0.0, end: 15.0).animate(
      CurvedAnimation(
        parent: _animationController,
        curve: Curves.easeOut,
        reverseCurve: Curves.easeIn,
      ),
    );

    if (widget.visible) {
      _animationController.forward();
    }
  }

  @override
  void dispose() {
    if (instance == this) {
      instance = null;
    }
    _animationController.dispose();
    searchFocusNode.dispose();
    searchController.dispose();
    hoveredIdNotifier.dispose();
    selectedIdNotifier.dispose();
    super.dispose();
  }

  @override
  void didUpdateWidget(covariant UIFTabScreen oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.visible && !widget.visible) {
      searchController.clear();
      searchText = "";
      searchFocusNode.unfocus();
      _animationController.reverse();
    }
    if (!oldWidget.visible && widget.visible) {
      selectedIdNotifier.value = -1;
      selectFirstIfNone();
      _animationController.forward();
    }
  }

  void selectFirstIfNone() {
    if (selectedIdNotifier.value == -1) {
      final list = filteredPlayers;
      if (list.isNotEmpty) {
        selectedIdNotifier.value = list.first.id;
      }
    }
  }

  void updatePlayerDataOnly(
      int id, String name, int score, int ping, int color) {
    if (id == localPlayerId) {
      ScoreBoardState.loadplayername.value = name;
    }
    players[id] = PlayerData(
      id: id,
      name: name,
      score: score,
      ping: ping,
      color: Color(color),
    );
  }

  void syncPlayers(Set<int> currentIds) {
    players.removeWhere((id, _) => !currentIds.contains(id));
  }

  void refreshUI() {
    if (mounted) setState(() {});
  }

  List<PlayerData> get filteredPlayers {
    final text = searchText.trim().toLowerCase();
    final list = players.values.toList();

    list.sort((a, b) {
      if (a.id == localPlayerId) return -1;
      if (b.id == localPlayerId) return 1;

      // Fallback to config name string matching if local ID is not set yet
      if (localPlayerId == -1) {
        final myName =
            ScoreBoardState.loadplayername.value.trim().toLowerCase();
        final aName = a.name.trim().toLowerCase();
        final bName = b.name.trim().toLowerCase();
        if (aName == myName) return -1;
        if (bName == myName) return 1;
      }

      return a.id.compareTo(b.id);
    });

    if (text.isEmpty) return list;
    return list
        .where((p) =>
            p.name.toLowerCase().contains(text) ||
            p.id.toString().contains(text))
        .toList();
  }

  @override
  Widget build(BuildContext context) {
    final media = MediaQuery.of(context);
    final insets = EdgeInsets.fromLTRB(media.padding.left, media.padding.top,
        media.padding.right, media.padding.bottom + media.viewInsets.bottom);
    final size = Size(math.max(1, media.size.width - insets.horizontal),
        math.max(1, media.size.height - insets.vertical));
    final bool isLandscape = size.width > size.height;
    scale = ((isLandscape ? size.width / 850 : size.width / 420) * 0.64)
        .clamp(0.48, 0.74);
    final panelWidth = size.width * (isLandscape ? 0.70 : 0.80);
    final panelHeight = size.height * (isLandscape ? 0.80 : 0.72);
    // Keep the original columns and local-player badge within narrow iOS viewports.
    scale = math.min(scale, math.min(panelWidth / 700, panelHeight / 200));

    final playersList = filteredPlayers;

    return Scaffold(
      resizeToAvoidBottomInset: false,
      backgroundColor: Colors.transparent,
      body: AnimatedBuilder(
        animation: _animationController,
        builder: (context, child) {
          final double opacityVal = _opacityAnimation.value;
          final double scaleVal = _scaleAnimation.value;
          final Offset slideVal = _slideAnimation.value;
          final double blurVal = _blurAnimation.value;

          return Container(
            color: Colors.black.withOpacity(0.45 * opacityVal),
            child: Stack(
              children: [
                Positioned.fill(
                  child: GestureDetector(
                    behavior: HitTestBehavior.translucent,
                    onTap: () {
                      onBackgroundTap();
                    },
                    child: const SizedBox.expand(),
                  ),
                ),
                Padding(
                  padding: insets,
                  child: Center(
                    child: Transform.translate(
                      offset: Offset(0, slideVal.dy * size.height),
                      child: Transform.scale(
                        scale: scaleVal,
                        child: Opacity(
                          opacity: opacityVal,
                          child: Container(
                            decoration: BoxDecoration(
                              borderRadius: BorderRadius.circular(rs(18)),
                              boxShadow: [
                                BoxShadow(
                                  color: Colors.black
                                      .withOpacity(0.65 * opacityVal),
                                  blurRadius: rs(28),
                                  spreadRadius: rs(4),
                                  offset: Offset(0, rs(10)),
                                ),
                                BoxShadow(
                                  color: const Color(0xFFFFB800)
                                      .withOpacity(0.03 * opacityVal),
                                  blurRadius: rs(20),
                                  spreadRadius: rs(1),
                                ),
                              ],
                            ),
                            child: GestureDetector(
                              onTap: () {
                                searchFocusNode.unfocus();
                              },
                              child: ClipRRect(
                                borderRadius: BorderRadius.circular(rs(18)),
                                child: BackdropFilter(
                                  filter: ImageFilter.blur(
                                    sigmaX: rs(blurVal),
                                    sigmaY: rs(blurVal),
                                  ),
                                  child: Container(
                                    width: isLandscape
                                        ? size.width * 0.70
                                        : size.width * 0.80,
                                    height: isLandscape
                                        ? size.height * 0.80
                                        : size.height * 0.72,
                                    decoration: BoxDecoration(
                                      borderRadius:
                                          BorderRadius.circular(rs(18)),
                                      border: Border.all(
                                        color: Colors.white.withOpacity(0.08),
                                        width: rs(1.2),
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
                                    child: Column(
                                      children: [
                                        buildTopBar(),
                                        Expanded(
                                          child: Container(
                                            decoration: BoxDecoration(
                                              border: Border.all(
                                                color: Colors.white
                                                    .withOpacity(0.05),
                                                width: rs(1),
                                              ),
                                              borderRadius: BorderRadius.only(
                                                bottomLeft:
                                                    Radius.circular(rs(18)),
                                                bottomRight:
                                                    Radius.circular(rs(18)),
                                              ),
                                            ),
                                            child: Column(
                                              children: [
                                                buildInfoHeader(),
                                                buildHeaders(), // Aligned columns headers
                                                Expanded(
                                                  child: Padding(
                                                    padding:
                                                        EdgeInsets.symmetric(
                                                      horizontal: rs(24),
                                                    ),
                                                    child: ListView.builder(
                                                      itemExtent: rs(52),
                                                      itemCount:
                                                          playersList.length,
                                                      itemBuilder:
                                                          (context, i) {
                                                        if (i >=
                                                            playersList
                                                                .length) {
                                                          return const SizedBox
                                                              .shrink();
                                                        }
                                                        final player =
                                                            playersList[i];
                                                        return _PlayerRowItem(
                                                          key: ValueKey(
                                                              player.id),
                                                          player: player,
                                                          localPlayerId:
                                                              localPlayerId,
                                                          scale: scale,
                                                          selectedIdNotifier:
                                                              selectedIdNotifier,
                                                          hoveredIdNotifier:
                                                              hoveredIdNotifier,
                                                          onTap: () {
                                                            searchFocusNode
                                                                .unfocus();
                                                            selectedIdNotifier
                                                                    .value =
                                                                player.id;
                                                          },
                                                          onDoubleTap: () {
                                                            ScoreBoardBridge
                                                                .sendTab(
                                                                    player.id);
                                                            print(
                                                                "Double tapped on: ${player.name}");
                                                          },
                                                        );
                                                      },
                                                    ),
                                                  ),
                                                ),
                                              ],
                                            ),
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
                  ),
                ),
              ],
            ),
          );
        },
      ),
    );
  }

  void onBackgroundTap() {
    searchFocusNode.unfocus();
    ScoreBoardBridge.Tabclose();
  }

  Widget buildTopBar() => Container(
        width: double.infinity,
        height: rs(64),
        decoration: BoxDecoration(
          color: Colors.transparent,
          border: Border(
            bottom: BorderSide(
              color: const Color(0xFFFFB800).withOpacity(0.3),
              width: rs(1.2),
            ),
          ),
        ),
        padding: EdgeInsets.symmetric(horizontal: rs(22)),
        child: Row(
          children: [
            Container(
              padding: EdgeInsets.all(rs(6)),
              height: rs(30),
              width: rs(30),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.06),
                borderRadius: BorderRadius.circular(rs(6)),
                border: Border.all(
                    color: Colors.white.withOpacity(0.1), width: 0.5),
              ),
              child: Icon(
                Icons.bar_chart_rounded,
                color: const Color(0xFFFFB800),
                size: rs(16),
              ),
            ),
            SizedBox(width: rs(14)),
            ValueListenableBuilder<String>(
              valueListenable: serverName,
              builder: (context, value, child) {
                return Text.rich(
                  TextSpan(
                    children: _parseSampText(value),
                  ),
                  style: _scoreBoardFont.copyWith(
                    fontSize: rs(20),
                    fontWeight: FontWeight.w800,
                  ),
                );
              },
            ),
            const Spacer(),
            Container(
              padding:
                  EdgeInsets.symmetric(horizontal: rs(12), vertical: rs(6)),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.04),
                borderRadius: BorderRadius.circular(rs(20)),
                border: Border.all(
                    color: Colors.white.withOpacity(0.08), width: 0.5),
              ),
              child: Row(
                children: [
                  Container(
                    width: rs(6),
                    height: rs(6),
                    decoration: const BoxDecoration(
                        color: Color(0xFF10B981),
                        shape: BoxShape.circle,
                        boxShadow: [
                          BoxShadow(
                            color: Color(0xFF10B981),
                            blurRadius: 4,
                            spreadRadius: 1,
                          )
                        ]),
                  ),
                  SizedBox(width: rs(8)),
                  ValueListenableBuilder<int>(
                    valueListenable: playerOnline,
                    builder: (context, value, child) {
                      return Text(
                        "$value ผู้เล่น",
                        style: _scoreBoardFont.copyWith(
                          color: Colors.white.withOpacity(0.8),
                          fontSize: rs(11),
                          fontWeight: FontWeight.w800,
                          letterSpacing: 0.5,
                        ),
                      );
                    },
                  ),
                ],
              ),
            ),
          ],
        ),
      );

  Widget buildInfoHeader() => Padding(
        padding: EdgeInsets.fromLTRB(rs(24), rs(14), rs(24), rs(8)),
        child: Row(
          children: [
            Icon(Icons.info_outline_rounded,
                color: const Color(0xFFFFB800), size: rs(24)),
            SizedBox(width: rs(8)),
            Text('ข้อมูล',
                style: _scoreBoardFont.copyWith(
                    color: const Color(0xFFFFB800),
                    fontSize: rs(16),
                    fontWeight: FontWeight.bold)),
            const Spacer(),
            Container(
              width: rs(240),
              height: rs(38),
              padding: EdgeInsets.symmetric(horizontal: rs(10)),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.03),
                borderRadius: BorderRadius.circular(rs(10)),
                border: Border.all(color: Colors.white.withOpacity(0.08)),
              ),
              child: Row(
                children: [
                  GestureDetector(
                    behavior: HitTestBehavior.opaque,
                    onTap: () {
                      if (searchText.isNotEmpty) {
                        searchController.clear();
                        setState(() {
                          searchText = "";
                        });
                      } else {
                        searchFocusNode.requestFocus();
                      }
                    },
                    child: Padding(
                      padding: EdgeInsets.symmetric(horizontal: rs(2)),
                      child: Icon(
                        searchText.isEmpty
                            ? Icons.search_rounded
                            : Icons.close_rounded,
                        color: const Color(0xFFFFB800),
                        size: rs(18),
                      ),
                    ),
                  ),
                  SizedBox(width: rs(6)),
                  Expanded(
                    child: TextField(
                      controller: searchController,
                      focusNode: searchFocusNode,
                      textAlignVertical: TextAlignVertical.center,
                      style: _scoreBoardFont.copyWith(
                        color: Colors.white,
                        fontSize: rs(14),
                      ),
                      onChanged: (value) {
                        setState(() {
                          searchText = value;
                        });
                      },
                      decoration: InputDecoration(
                        border: InputBorder.none,
                        isCollapsed: true,
                        hintText: 'ค้นหาผู้เล่น...',
                        hintStyle: _scoreBoardFont.copyWith(
                          color: Colors.white30,
                          fontSize: rs(13),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      );

  Widget buildHeaders() => Padding(
        padding: EdgeInsets.symmetric(horizontal: rs(38), vertical: rs(8)),
        child: Row(
          children: [
            SizedBox(
              width: rs(110),
              child: Text(
                "ID",
                style: _scoreBoardFont.copyWith(
                  color: Colors.white24,
                  fontSize: rs(11),
                  fontWeight: FontWeight.bold,
                  letterSpacing: 0.5,
                ),
              ),
            ),
            SizedBox(width: rs(24)),
            Expanded(
              child: Text(
                "PLAYER NAME",
                style: _scoreBoardFont.copyWith(
                  color: Colors.white24,
                  fontSize: rs(11),
                  fontWeight: FontWeight.bold,
                  letterSpacing: 0.5,
                ),
              ),
            ),
            SizedBox(
              width: rs(180),
              child: Text(
                "SCORE",
                style: _scoreBoardFont.copyWith(
                  color: Colors.white24,
                  fontSize: rs(11),
                  fontWeight: FontWeight.bold,
                  letterSpacing: 0.5,
                ),
              ),
            ),
            SizedBox(
              width: rs(120),
              child: Text(
                "PING",
                style: _scoreBoardFont.copyWith(
                  color: Colors.white24,
                  fontSize: rs(11),
                  fontWeight: FontWeight.bold,
                  letterSpacing: 0.5,
                ),
              ),
            ),
          ],
        ),
      );
}

// Optimized Player Row Widget to achieve O(1) rebuilds on hover and tap
class _PlayerRowItem extends StatelessWidget {
  final PlayerData player;
  final int localPlayerId;
  final double scale;
  final ValueNotifier<int> selectedIdNotifier;
  final ValueNotifier<int> hoveredIdNotifier;
  final VoidCallback onTap;
  final VoidCallback onDoubleTap;

  const _PlayerRowItem({
    super.key,
    required this.player,
    required this.localPlayerId,
    required this.scale,
    required this.selectedIdNotifier,
    required this.hoveredIdNotifier,
    required this.onTap,
    required this.onDoubleTap,
  });

  double rs(double size) => size * scale;

  @override
  Widget build(BuildContext context) {
    final isMe = player.id == localPlayerId ||
        (localPlayerId == -1 &&
            player.name.trim().toLowerCase() ==
                ScoreBoardState.loadplayername.value.trim().toLowerCase());

    return MouseRegion(
      onEnter: (_) => hoveredIdNotifier.value = player.id,
      onExit: (_) => hoveredIdNotifier.value = -1,
      child: GestureDetector(
        onTap: onTap,
        onDoubleTap: onDoubleTap,
        child: ValueListenableBuilder<int>(
          valueListenable: selectedIdNotifier,
          builder: (context, selectedId, _) {
            final isSelected = selectedId == player.id;

            return ValueListenableBuilder<int>(
              valueListenable: hoveredIdNotifier,
              builder: (context, hoveredId, _) {
                final isHovered = hoveredId == player.id;

                Color pingColor = Colors.white54;
                IconData pingIcon = Icons.signal_cellular_alt_rounded;
                if (player.ping < 80) {
                  pingColor = const Color(0xFF10B981);
                } else if (player.ping < 180) {
                  pingColor = const Color(0xFFF59E0B);
                } else {
                  pingColor = const Color(0xFFEF4444);
                }

                return AnimatedContainer(
                  duration: const Duration(milliseconds: 150),
                  height: rs(42),
                  margin: EdgeInsets.only(bottom: rs(10)),
                  padding: EdgeInsets.symmetric(horizontal: rs(14)),
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(rs(10)),
                    border: Border.all(
                      color: isSelected
                          ? const Color(0xFFFFB800)
                          : isMe
                              ? const Color(0xFF10B981).withOpacity(0.3)
                              : isHovered
                                  ? Colors.white.withOpacity(0.15)
                                  : Colors.white.withOpacity(0.04),
                      width: 1.0,
                    ),
                    color: isSelected
                        ? const Color(0xFFFFB800).withOpacity(0.12)
                        : isMe
                            ? const Color(0xFF10B981).withOpacity(0.08)
                            : isHovered
                                ? Colors.white.withOpacity(0.04)
                                : Colors.white.withOpacity(0.015),
                  ),
                  child: Row(
                    children: [
                      if (isSelected)
                        Container(
                          width: rs(3),
                          height: rs(16),
                          margin: EdgeInsets.only(right: rs(8)),
                          decoration: BoxDecoration(
                            color: const Color(0xFFFFB800),
                            borderRadius: BorderRadius.circular(rs(2)),
                          ),
                        )
                      else if (isMe)
                        Container(
                          width: rs(3),
                          height: rs(16),
                          margin: EdgeInsets.only(right: rs(8)),
                          decoration: BoxDecoration(
                            color: const Color(0xFF10B981),
                            borderRadius: BorderRadius.circular(rs(2)),
                          ),
                        )
                      else
                        SizedBox(width: rs(3)),
                      SizedBox(
                        width: rs(107),
                        child: Text(
                          "# ${player.id}",
                          style: _scoreBoardFont.copyWith(
                            color: isSelected
                                ? const Color(0xFFFFB800)
                                : player.color == Colors.white
                                    ? Colors.white70
                                    : player.color,
                            fontSize: rs(15),
                            fontWeight: isMe || isSelected
                                ? FontWeight.bold
                                : FontWeight.w500,
                          ),
                        ),
                      ),
                      Container(
                        width: rs(8),
                        height: rs(8),
                        decoration: BoxDecoration(
                          color: player.color,
                          shape: BoxShape.circle,
                          boxShadow: [
                            BoxShadow(
                              color: player.color.withOpacity(0.5),
                              blurRadius: rs(4),
                              spreadRadius: rs(1),
                            ),
                          ],
                        ),
                      ),
                      SizedBox(width: rs(14)),
                      Expanded(
                        child: Row(
                          children: [
                            Flexible(
                              child: Text(
                                player.name,
                                overflow: TextOverflow.ellipsis,
                                style: _scoreBoardFont.copyWith(
                                  color: isSelected
                                      ? const Color(0xFFFFB800)
                                      : player.color == Colors.white
                                          ? Colors.white.withOpacity(0.9)
                                          : player.color,
                                  fontSize: rs(16),
                                  fontWeight: isMe || isSelected
                                      ? FontWeight.bold
                                      : FontWeight.w500,
                                ),
                              ),
                            ),
                            if (isMe) ...[
                              SizedBox(width: rs(8)),
                              Container(
                                padding: EdgeInsets.symmetric(
                                    horizontal: rs(6), vertical: rs(2)),
                                decoration: BoxDecoration(
                                  color:
                                      const Color(0xFF10B981).withOpacity(0.18),
                                  borderRadius: BorderRadius.circular(rs(4)),
                                  border: Border.all(
                                      color: const Color(0xFF10B981)
                                          .withOpacity(0.3),
                                      width: 0.5),
                                ),
                                child: Text(
                                  'คุณ',
                                  style: _scoreBoardFont.copyWith(
                                    color: const Color(0xFF10B981),
                                    fontSize: rs(9),
                                    fontWeight: FontWeight.bold,
                                    letterSpacing: 0.5,
                                  ),
                                ),
                              ),
                            ],
                          ],
                        ),
                      ),
                      Icon(Icons.trending_up_rounded,
                          color: Colors.white24, size: rs(14)),
                      SizedBox(width: rs(10)),
                      SizedBox(
                        width: rs(156),
                        child: Text(
                          "${player.score}",
                          style: _scoreBoardFont.copyWith(
                            color: isSelected
                                ? const Color(0xFFFFB800)
                                : Colors.white70,
                            fontSize: rs(15),
                            fontWeight: isSelected
                                ? FontWeight.bold
                                : FontWeight.normal,
                          ),
                        ),
                      ),
                      Icon(pingIcon,
                          color: pingColor.withOpacity(0.6), size: rs(14)),
                      SizedBox(width: rs(10)),
                      SizedBox(
                        width: rs(96),
                        child: Text(
                          "${player.ping}",
                          style: _scoreBoardFont.copyWith(
                            color: isSelected
                                ? const Color(0xFFFFB800)
                                : Colors.white70,
                            fontSize: rs(15),
                            fontWeight: isSelected
                                ? FontWeight.bold
                                : FontWeight.normal,
                          ),
                        ),
                      ),
                    ],
                  ),
                );
              },
            );
          },
        ),
      ),
    );
  }
}

class SampScoreBoardOverlay extends StatefulWidget {
  const SampScoreBoardOverlay({super.key});

  @override
  State<SampScoreBoardOverlay> createState() => _SampScoreBoardOverlayState();
}

class _SampScoreBoardOverlayState extends State<SampScoreBoardOverlay> {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) async {
      if (mounted) {
        await ScoreBoardBridge.channel.invokeMethod<bool>('ready');
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder<bool>(
      valueListenable: ScoreBoardState.visible,
      builder: (context, visible, child) => IgnorePointer(
        ignoring: !visible,
        child: ExcludeSemantics(
          excluding: !visible,
          child: Opacity(
            opacity: visible ? 1 : 0,
            child: UIFTabScreen(visible: visible),
          ),
        ),
      ),
    );
  }
}
