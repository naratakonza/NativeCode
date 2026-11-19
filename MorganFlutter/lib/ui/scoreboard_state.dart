import 'package:flutter/foundation.dart';

// Scoreboard visibility and local-player name for the iOS overlay.
class ScoreBoardState {
  static final visible = ValueNotifier<bool>(false);
  static final loadplayername = ValueNotifier<String>('');
}
