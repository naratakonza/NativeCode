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

    await tester.tapAt(const Offset(140, 30));
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 300));
    await tester.enterText(find.byType(TextField), '/rec');
    await tester.tap(find.byTooltip('ส่ง'));
    await tester.pump();
    final submission = calls.singleWhere((call) => call.method == 'sendText');
    expect(submission.arguments, {'text': '/rec'});
    expect(tester.widget<TextField>(find.byType(TextField)).enabled, isFalse);

    await native('hide');
    expect(find.byType(TextField), findsNothing);
    final region = calls.lastWhere((call) => call.method == 'hitRegion');
    expect((region.arguments as Map)['width'], 0);
    messenger.setMockMethodCallHandler(channel, null);
  });
}
