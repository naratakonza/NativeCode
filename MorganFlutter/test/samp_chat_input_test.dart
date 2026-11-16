import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:morgan_loading/ui/samp_chat.dart';

Future<void> _send(WidgetTester tester, String method, [Object? arguments]) {
  return tester.binding.defaultBinaryMessenger.handlePlatformMessage(
    SampChatController.channel.name,
    const StandardMethodCodec().encodeMethodCall(MethodCall(method, arguments)),
    (_) {},
  );
}

void main() {
  testWidgets('opens, dismisses and reopens chat input across a pause',
      (tester) async {
    final calls = <MethodCall>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      SampChatController.channel,
      (call) async {
        calls.add(call);
        return true;
      },
    );

    await tester.pumpWidget(const MaterialApp(home: SampChat()));
    await _send(tester, 'show');
    await tester.pumpAndSettle();

    final region = calls.lastWhere((call) => call.method == 'hitRegion')
        .arguments as Map;
    final tap = Offset(region['x'] + 20.0, region['y'] + 20.0);
    final field = find.byType(TextField);

    await tester.tapAt(tap);
    await tester.pumpAndSettle();
    expect(tester.widget<TextField>(field).focusNode!.hasFocus, isTrue);
    expect(tester.testTextInput.isVisible, isTrue);

    await _send(tester, 'HideInputChat');
    await tester.pumpAndSettle();
    expect(field, findsNothing);
    expect(tester.testTextInput.isVisible, isFalse);

    await _send(tester, 'setPauseState', true);
    await _send(tester, 'setPauseState', false);
    await tester.pumpAndSettle();
    await tester.tapAt(tap);
    await tester.pumpAndSettle();
    expect(tester.widget<TextField>(field).focusNode!.hasFocus, isTrue);
    expect(calls.where((call) => call.method == 'inputActive')
        .map((call) => call.arguments), [true, false, true]);
  });

  testWidgets('closing before the first frame cancels the focus request',
      (tester) async {
    final key = GlobalKey<SampChatState>();
    await tester.pumpWidget(MaterialApp(home: SampChat(key: key)));
    await _send(tester, 'show');
    await tester.pumpAndSettle();

    key.currentState!.showInput();
    await _send(tester, 'HideInputChat');
    await tester.pumpAndSettle();
    expect(find.byType(TextField), findsNothing);
    expect(tester.testTextInput.isVisible, isFalse);
  });
}
