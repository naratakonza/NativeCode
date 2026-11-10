import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:morgan_loading/ui/samp_chat.dart';

void main() {
  testWidgets('receives colored chat and sends a command through flutter_chat', (tester) async {
    const channel = SampChatController.channel;
    const codec = StandardMethodCodec();
    final calls = <MethodCall>[];
    final messenger = tester.binding.defaultBinaryMessenger;
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return true;
    });

    Future<void> native(String method, [Object? arguments]) async {
      await messenger.handlePlatformMessage(
        channel.name,
        codec.encodeMethodCall(MethodCall(method, arguments)),
        (_) {},
      );
      await tester.pump();
    }

    await tester.pumpWidget(const MaterialApp(home: SampChat()));
    expect(calls.any((call) => call.method == 'ready'), isTrue);
    await native('ChatPosition', {'position': 0, 'radarRight': 280});
    await native('show');
    Map<Object?, Object?> hitRegion() =>
        calls.lastWhere((call) => call.method == 'hitRegion').arguments as Map<Object?, Object?>;
    final area = hitRegion();
    expect(area['x'], greaterThan(224));
    expect((area['x'] as num) + (area['width'] as num), lessThanOrEqualTo(796));
    await native('addMessage', {'message': 'Welcome {FFCC00}Morgan', 'color': '{FFFFFF}', 'type': 0});
    expect(find.textContaining('Welcome', findRichText: true), findsWidgets);
    final colored = tester.widgetList<RichText>(find.byType(RichText)).any((widget) {
      var found = false;
      widget.text.visitChildren((span) {
        found |= span is TextSpan && span.text == 'Morgan' && span.style?.color == const Color(0xffffcc00);
        return !found;
      });
      return found;
    });
    expect(colored, isTrue);

    final chatPoint = Offset((area['x'] as num).toDouble() + 20, 30);
    await tester.tapAt(chatPoint);
    await tester.pump();
    expect(tester.testTextInput.isVisible, isTrue);
    expect(calls.lastWhere((call) => call.method == 'inputActive').arguments, isTrue);
    await tester.enterText(find.byType(TextField), '/rec');
    await native('HideInputChat');
    expect(tester.testTextInput.isVisible, isFalse);
    expect(calls.lastWhere((call) => call.method == 'inputActive').arguments, isFalse);

    await tester.tapAt(chatPoint);
    await tester.pump();
    expect(tester.testTextInput.isVisible, isTrue);
    expect(tester.widget<TextField>(find.byType(TextField)).controller?.text, '/rec');
    await tester.tap(find.byTooltip('ส่ง'));
    await tester.pump();
    final submission = calls.singleWhere((call) => call.method == 'sendText');
    expect(submission.arguments, {'text': '/rec'});
    expect(find.byType(TextField), findsNothing);

    await tester.tapAt(chatPoint);
    await tester.pump();
    expect(tester.testTextInput.isVisible, isTrue);
    await native('setPauseState', true);
    expect(tester.testTextInput.isVisible, isFalse);
    expect(find.byType(TextField), findsNothing);
    expect(hitRegion()['width'], 0);
    await native('show');
    expect(hitRegion()['width'], 0);
    await native('ChatPosition', {'position': 0, 'radarRight': -1000});
    await native('setPauseState', false);
    expect(hitRegion()['x'], lessThan(40));
    expect(find.textContaining('Welcome', findRichText: true), findsWidgets);
    await native('hide');
    expect(hitRegion()['width'], 0);
    messenger.setMockMethodCallHandler(channel, null);
  });

  testWidgets('captures chat builds before and after the iOS keyboard inset changes', (tester) async {
    addTearDown(tester.view.resetViewInsets);
    const channel = SampChatController.channel;
    final calls = <MethodCall>[];
    final messenger = tester.binding.defaultBinaryMessenger;
    var inputReady = Completer<bool>();
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      if (call.method == 'inputActive' && call.arguments == true) {
        return inputReady.future;
      }
      return true;
    });
    addTearDown(() => messenger.setMockMethodCallHandler(channel, null));

    await tester.pumpWidget(const MaterialApp(home: SampChat()));
    await messenger.handlePlatformMessage(
      channel.name,
      const StandardMethodCodec().encodeMethodCall(const MethodCall('show')),
      (_) {},
    );
    await tester.pump();
    await tester.tapAt(const Offset(250, 30));
    await tester.pump();
    final start = calls.singleWhere((call) => call.method == 'keyboardProbeStart');
    expect(tester.testTextInput.isVisible, isFalse);
    inputReady.complete(true);
    await tester.pump();
    expect(tester.testTextInput.isVisible, isTrue);

    tester.view.viewInsets = FakeViewPadding(bottom: 180 * tester.view.devicePixelRatio);
    await tester.pump();
    await tester.pump(const Duration(seconds: 4));
    final sample = calls.singleWhere((call) => call.method == 'keyboardProbeEnd').arguments as Map;
    expect(sample['start'], start.arguments);
    expect(sample['focus'], isA<int>());
    expect((sample['layouts'] as List).map((row) => (row as List)[2]), containsAllInOrder([0, 1800]));

    await tester.tapAt(const Offset(250, 30));
    await tester.pump();
    inputReady = Completer<bool>();
    await tester.tapAt(const Offset(250, 30));
    await tester.pump();
    await messenger.handlePlatformMessage(
      channel.name,
      const StandardMethodCodec().encodeMethodCall(const MethodCall('HideInputChat')),
      (_) {},
    );
    await tester.pump();
    inputReady.complete(true);
    await tester.pump();
    expect(tester.testTextInput.isVisible, isFalse);
    await tester.pumpWidget(const SizedBox.shrink());
  }, variant: const TargetPlatformVariant({TargetPlatform.iOS}));
}
