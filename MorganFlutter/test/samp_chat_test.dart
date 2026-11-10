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
    await native('show');
    await native('addMessage', {'message': 'Welcome {FFCC00}Morgan', 'color': '{FFFFFF}', 'type': 0});
    final message = tester.widget<Text>(find.textContaining('Welcome'));
    final spans = (message.textSpan! as TextSpan).children!.cast<TextSpan>();
    expect(spans.last.text, 'Morgan');
    expect(spans.last.style?.color, const Color(0xffffcc00));

    await tester.tap(find.byTooltip('เปิดแชท'));
    await tester.pump();
    await tester.enterText(find.byType(TextField), '/rec');
    await tester.tap(find.byTooltip('ส่ง'));
    await tester.pump();
    final submission = calls.singleWhere((call) => call.method == 'sendText');
    expect(submission.arguments, {'text': '/rec'});
    expect(find.byType(TextField), findsNothing);

    await native('hide');
    expect(find.byTooltip('เปิดแชท'), findsNothing);
    final region = calls.lastWhere((call) => call.method == 'hitRegion');
    expect((region.arguments as Map)['width'], 0);
    messenger.setMockMethodCallHandler(channel, null);
  });
}
