import 'package:flutter/material.dart';

import 'ui/dialog.dart';
import 'ui/game_loading_screen.dart';
import 'ui/samp_chat.dart';
import 'ui/spawn.dart';
import 'ui/keys_bar.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SpawnBridge.init();
  DialogBridge.init();
  KeysBarBridge.init();
  runApp(const SAGameApp());
}

class SAGameApp extends StatelessWidget {
  const SAGameApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      builder: (context, child) {
        return MediaQuery(
          data: MediaQuery.of(
            context,
          ).copyWith(textScaler: TextScaler.noScaling),
          child: child!,
        );
      },
      home: const Stack(
        fit: StackFit.expand,
        children: [
          SampChat(),
          SampSpawnMenu(),
          SampKeysBarOverlay(),
          SampDialogOverlay(),
          IgnorePointer(child: GameLoadingScreen(isVisible: false)),
        ],
      ),
    );
  }
}
