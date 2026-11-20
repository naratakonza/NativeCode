import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:morgan_loading/ui/keys_bar.dart';
import 'package:morgan_loading/ui/samp_chat.dart';

Future<void> sendChat(WidgetTester tester, String method, [Object? args]) =>
    tester.binding.defaultBinaryMessenger.handlePlatformMessage(
      SampChatController.channel.name,
      const StandardMethodCodec().encodeMethodCall(MethodCall(method, args)),
      (_) {},
    );

Rect lastRegion(List<MethodCall> calls) {
  final data = calls.lastWhere((c) => c.method == 'hitRegion').arguments as Map;
  return Rect.fromLTWH(
    (data['x'] as num).toDouble(),
    (data['y'] as num).toDouble(),
    (data['width'] as num).toDouble(),
    (data['height'] as num).toDouble(),
  );
}

void main() {
  for (final size in <Size>[
    const Size(844, 390),
    const Size(926, 428),
    const Size(1024, 768),
    const Size(1194, 834),
    const Size(1366, 1024),
    const Size(768, 1024),
  ]) {
    testWidgets('positions and sizes at $size', (tester) async {
      final tablet = size.shortestSide >= 600;
      final safe = tablet
          ? const EdgeInsets.only(top: 24, bottom: 20)
          : const EdgeInsets.only(left: 47, right: 47, bottom: 21);
      tester.view.devicePixelRatio = 2;
      tester.view.physicalSize = size * 2;
      tester.view.viewPadding = FakeViewPadding(
        left: safe.left * 2,
        top: safe.top * 2,
        right: safe.right * 2,
        bottom: safe.bottom * 2,
      );
      addTearDown(tester.view.reset);
      final chatCalls = <MethodCall>[];
      final keyCalls = <MethodCall>[];
      tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
        SampChatController.channel,
        (call) async {
          chatCalls.add(call);
          return true;
        },
      );
      tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
        KeysBarBridge.channel,
        (call) async {
          keyCalls.add(call);
          return true;
        },
      );
      KeysBarBridge.state.value = {
        'visible': true,
        'radarLeft': 20,
        'radarBottom': tablet ? 240 : 340,
      };
      await tester.pumpWidget(
        const MaterialApp(
          home: Stack(
            fit: StackFit.expand,
            children: [SampChat(), SampKeysBarOverlay()],
          ),
        ),
      );
      await sendChat(tester, 'show');
      await tester.pumpAndSettle();
      final chat = lastRegion(chatCalls);
      final keys = lastRegion(keyCalls);
      final landscape = size.width > size.height;
      final chatScale =
          (landscape ? size.width / 850 : size.width / 420).clamp(0.85, 1.3) *
          0.8;
      final keyScale = math.min(
        (size.width - safe.horizontal) / 970,
        (size.height - safe.vertical) / 540,
      );
      final height = (landscape ? 175 : 190) * chatScale;
      final gap = math.max(10.0, 12 * keyScale);
      expect(chat.height, closeTo(height, 0.01));
      expect(keys.height, closeTo(41 * keyScale, 0.01));
      final infoLeft = safe.left + (size.width - safe.horizontal) * 4 / 640;
      final toggle = tester.getRect(find.ancestor(
        of: find.text('>>'),
        matching: find.byType(GestureDetector),
      ).first);
      expect(keys.left, closeTo(infoLeft, 0.01));
      expect(toggle.left, closeTo(infoLeft, 0.01));
      expect(
        tester.widget<Text>(find.text('>>')).style!.fontSize,
        closeTo(11.25 * keyScale, 0.01),
      );
      expect(keys.top, greaterThan(size.height * (tablet ? 0.240 : 0.340)));
      if (tablet) {
        expect(keys.bottom, closeTo(size.height * 0.45 - gap, 0.01));
        expect(chat.top, closeTo(safe.top + 4 * chatScale, 0.01));
        expect(chat.bottom, lessThan(keys.top));
      } else {
        expect(chat.top, closeTo(safe.top + 4 * chatScale, 0.01));
        expect(
          keys.top,
          closeTo(math.max(safe.top + gap, size.height * 0.340 + gap), 0.01),
        );
      }
      for (var i = 0; i < 6; i++) {
        await sendChat(tester, 'addMessage', {
          'message': 'Chat position check $i',
          'type': 0,
        });
      }
      await tester.pumpAndSettle();
      expect(lastRegion(chatCalls), chat);
      expect(lastRegion(keyCalls), keys);
      expect(tester.takeException(), isNull);
      await tester.pumpWidget(const SizedBox.shrink());
      await tester.pumpAndSettle();
    });
  }

  testWidgets('tablet keyboard hides Keybar and restores expanded state', (
    tester,
  ) async {
    tester.view.devicePixelRatio = 1;
    tester.view.physicalSize = const Size(1024, 768);
    addTearDown(tester.view.reset);
    final chatCalls = <MethodCall>[];
    final keyCalls = <MethodCall>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      SampChatController.channel,
      (call) async {
        chatCalls.add(call);
        return true;
      },
    );
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      KeysBarBridge.channel,
      (call) async {
        keyCalls.add(call);
        return true;
      },
    );
    KeysBarBridge.state.value = {
      'visible': true,
      'radarLeft': 20,
      'radarBottom': 240,
    };
    await tester.pumpWidget(
      const MaterialApp(
        home: Stack(
          fit: StackFit.expand,
          children: [SampChat(), SampKeysBarOverlay()],
        ),
      ),
    );
    await sendChat(tester, 'show');
    await tester.pumpAndSettle();
    await tester.tap(find.text('>>'));
    await tester.pumpAndSettle();
    final before = lastRegion(keyCalls);
    await tester.tapAt(lastRegion(chatCalls).topLeft + const Offset(20, 20));
    await tester.pumpAndSettle();
    expect(find.byType(TextField), findsOneWidget);
    expect(find.text('<<'), findsNothing);
    expect(lastRegion(keyCalls), Rect.zero);
    tester.view.viewInsets = const FakeViewPadding(bottom: 500);
    await tester.pumpAndSettle();
    expect(lastRegion(chatCalls).bottom, lessThanOrEqualTo(768 - 500));
    expect(tester.takeException(), isNull);
    await sendChat(tester, 'HideInputChat');
    tester.view.viewInsets = const FakeViewPadding();
    await tester.pumpAndSettle();
    expect(find.text('<<'), findsOneWidget);
    expect(lastRegion(keyCalls).top, closeTo(before.top, 0.01));
    await tester.pumpWidget(const SizedBox.shrink());
    await tester.pumpAndSettle();
  });
}
