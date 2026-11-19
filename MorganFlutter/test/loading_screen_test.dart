import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:morgan_loading/ui/game_loading_screen.dart';

const channel = MethodChannel('flutter_loadingscreen');
const codec = StandardMethodCodec();

Future<void> send(WidgetTester tester, String name, [Object? value]) async {
  await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
    channel.name,
    codec.encodeMethodCall(MethodCall(name, value)),
    (ByteData? reply) {},
  );
}

void main() {
  testWidgets('loads at landscape safe area and dismisses after native 100', (
    tester,
  ) async {
    final outbound = <String>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(channel, (
      call,
    ) async {
      outbound.add(call.method);
      return null;
    });
    await tester.pumpWidget(
      const MaterialApp(
        home: MediaQuery(
          data: MediaQueryData(
            size: Size(844, 390),
            viewPadding: EdgeInsets.fromLTRB(47, 0, 47, 21),
          ),
          child: GameLoadingScreen(isVisible: false),
        ),
      ),
    );
    expect(outbound, ['ready']);
    expect(find.text('กำลังโหลดข้อมูล'), findsNothing);

    await send(tester, 'show');
    await send(tester, 'updateStatusText', 'โหลดทรัพยากร');
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 600));
    expect(find.text('โหลดทรัพยากร'), findsOneWidget);
    expect(tester.getTopLeft(find.text('กำลังโหลดข้อมูล')).dx, greaterThan(47));

    await send(tester, 'updateProgress', 100);
    await send(tester, 'hide');
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 160));
    expect(find.text('100'), findsOneWidget);
    await tester.pump(const Duration(milliseconds: 260));
    await tester.pump(const Duration(milliseconds: 510));
    expect(find.text('กำลังโหลดข้อมูล'), findsNothing);
    expect(outbound, ['ready', 'onLoadingComplete']);
  });

  testWidgets('show cancels a pending hide and keeps the current session', (
    tester,
  ) async {
    await tester.pumpWidget(const MaterialApp(home: GameLoadingScreen()));
    await tester.pump(const Duration(milliseconds: 600));
    await send(tester, 'hide');
    await tester.pump(const Duration(milliseconds: 200));
    await send(tester, 'show');
    await tester.pump(const Duration(milliseconds: 600));
    expect(find.text('กำลังโหลดข้อมูล'), findsOneWidget);
  });
}
