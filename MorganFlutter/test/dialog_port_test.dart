import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:morgan_loading/ui/dialog.dart';

const codec = StandardMethodCodec();
const safePadding = EdgeInsets.fromLTRB(47, 0, 47, 21);

Future<void> send(WidgetTester tester, String name, [Object? args]) {
  return tester.binding.defaultBinaryMessenger.handlePlatformMessage(
    DialogBridge.channel.name,
    codec.encodeMethodCall(MethodCall(name, args)),
    (_) {},
  );
}

void main() {
  testWidgets('preserves the input over pause, fits the iOS keyboard and responds', (tester) async {
    await tester.binding.setSurfaceSize(const Size(667, 375));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    final outbound = <MethodCall>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      DialogBridge.channel,
      (call) async {
        outbound.add(call);
        return true;
      },
    );
    DialogBridge.init();
    final metrics = ValueNotifier(
      const MediaQueryData(size: Size(667, 375), viewPadding: safePadding),
    );
    await tester.pumpWidget(MaterialApp(
      home: ValueListenableBuilder<MediaQueryData>(
        valueListenable: metrics,
        builder: (context, data, child) => MediaQuery(data: data, child: child!),
        child: const SampDialogOverlay(),
      ),
    ));

    const data = {
      'id': 5,
      'sequence': 10,
      'style': 3,
      'title': 'รหัสผ่าน',
      'content': 'กรุณาใส่รหัสผ่าน',
      'button1': 'ยืนยัน',
      'button2': 'ยกเลิก',
    };
    await send(tester, 'showDialog', data);
    await send(tester, 'setDialogVisible', true);
    await tester.pumpAndSettle();
    final field = find.byType(TextField);
    expect(tester.widget<TextField>(field).obscureText, isTrue);
    expect(tester.getTopLeft(field).dx, greaterThan(safePadding.left));
    await tester.enterText(field, 'ทดสอบ');
    metrics.value = metrics.value.copyWith(viewInsets: const EdgeInsets.only(bottom: 270));
    await tester.pumpAndSettle();
    expect(tester.takeException(), isNull);

    await send(tester, 'setDialogVisible', false);
    await tester.pumpAndSettle();
    expect(field, findsNothing);
    metrics.value = metrics.value.copyWith(viewInsets: EdgeInsets.zero);
    await send(tester, 'setDialogVisible', true);
    await tester.pumpAndSettle();
    expect(tester.widget<TextField>(field).controller!.text, 'ทดสอบ');
    await tester.tap(find.text('ยืนยัน', findRichText: true));
    await tester.pump();
    expect(outbound.last.method, 'dialogResponse');
    expect(outbound.last.arguments, {
      'id': 5, 'sequence': 10, 'response': 1, 'listitem': -1, 'input': 'ทดสอบ',
    });
    await send(tester, 'hideDialog');
    await tester.pumpAndSettle();
    expect(field, findsNothing);
  });

  testWidgets('tablist headers send a zero based row and the first column', (tester) async {
    final outbound = <MethodCall>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      DialogBridge.channel,
      (call) async {
        outbound.add(call);
        return true;
      },
    );
    DialogBridge.init();
    await tester.pumpWidget(const MaterialApp(home: SampDialogOverlay()));
    await send(tester, 'showDialog', {
      'id': 7,
      'sequence': 11,
      'style': 5,
      'title': 'รายการ',
      'content': 'Name\tScore\n{FF0000}Alice\t5\nBob\t4',
      'button1': 'เลือก',
      'button2': '',
    });
    await send(tester, 'setDialogVisible', true);
    await tester.pump(const Duration(milliseconds: 200));
    await tester.tap(find.text('เลือก', findRichText: true));
    expect(outbound.last.arguments, {
      'id': 7, 'sequence': 11, 'response': 1, 'listitem': 0, 'input': 'Alice',
    });
    await send(tester, 'hideDialog');
    await tester.pumpAndSettle();
  });
}
