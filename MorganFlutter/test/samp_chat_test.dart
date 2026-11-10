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
    await tester.enterText(find.byType(TextField), '/rec');
    await tester.tap(find.byTooltip('ส่ง'));
    await tester.pump();
    final submission = calls.singleWhere((call) => call.method == 'sendText');
    expect(submission.arguments, {'text': '/rec'});
    expect(find.byType(TextField), findsNothing);

    await tester.tapAt(chatPoint);
    await tester.pump();
    expect(tester.testTextInput.isVisible, isTrue);
    await native('hide');
    expect(tester.testTextInput.isVisible, isFalse);
    expect(find.byType(TextField), findsNothing);
    expect(hitRegion()['width'], 0);
    await native('ChatPosition', {'position': 0, 'radarRight': -1000});
    await native('show');
    expect(hitRegion()['x'], lessThan(40));
    expect(find.textContaining('Welcome', findRichText: true), findsWidgets);
    messenger.setMockMethodCallHandler(channel, null);
  });
}
