import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:morgan_loading/ui/keys_bar.dart';

const _bar = ValueKey('samp_keys_bar');

Future<void> _native(WidgetTester tester, String method, [Object? args]) async {
  const codec = StandardMethodCodec();
  await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
    SampKeysBar.channel.name,
    codec.encodeMethodCall(MethodCall(method, args)),
    (_) {},
  );
  await tester.pump();
}

Widget _game(VoidCallback tap) {
  return MaterialApp(
    home: Stack(
      fit: StackFit.expand,
      children: [
        GestureDetector(behavior: HitTestBehavior.opaque, onTap: tap),
        const SampKeysBar(),
      ],
    ),
  );
}

void main() {
  testWidgets('shows below the radar and keeps game touches available',
      (tester) async {
    tester.view
      ..devicePixelRatio = 3
      ..physicalSize = const Size(2532, 1170)
      ..viewPadding = const FakeViewPadding(left: 141, right: 141, bottom: 63);
    addTearDown(tester.view.reset);
    var gameTaps = 0;
    await tester.pumpWidget(_game(() => gameTaps++));
    expect(find.byKey(_bar), findsNothing);

    await _native(tester, 'show', {'radarLeft': 20, 'radarBottom': 345});
    final bar = tester.getRect(find.byKey(_bar));
    expect(bar.left, 55);
    expect(bar.top, greaterThan(390 * 0.345 + 12));
    expect(bar.right, lessThanOrEqualTo(844 - 47 - 8));
    expect(tester.getTopLeft(find.text('TAB')).dy,
        tester.getTopLeft(find.text('2')).dy);
    expect(find.text('SHIFT'), findsOneWidget);
    await tester.tapAt(tester.getCenter(find.text('TAB')));
    expect(gameTaps, 1);

    await _native(tester, 'show', {'radarLeft': 150, 'radarBottom': 450});
    final moved = tester.getRect(find.byKey(_bar));
    expect(moved.left, closeTo(844 * 0.15, 0.01));
    expect(moved.top, greaterThan(bar.top));
    await _native(tester, 'hide');
    expect(find.byKey(_bar), findsNothing);
  });

  testWidgets('wraps within a narrow safe area and hides when it cannot fit',
      (tester) async {
    tester.view
      ..devicePixelRatio = 1
      ..physicalSize = const Size(390, 250)
      ..viewPadding = const FakeViewPadding(left: 20, right: 20, bottom: 16);
    addTearDown(tester.view.reset);
    await tester.pumpWidget(_game(() {}));
    await _native(tester, 'show', {'radarLeft': 0, 'radarBottom': 500});

    expect(tester.getTopLeft(find.text('2')).dy,
        greaterThan(tester.getTopLeft(find.text('TAB')).dy));
    final bar = tester.getRect(find.byKey(_bar));
    expect(bar.right, lessThanOrEqualTo(390 - 20 - 8));
    expect(bar.bottom, lessThanOrEqualTo(250 - 16 - 8));
    tester.view.viewInsets = const FakeViewPadding(bottom: 80);
    await tester.pump();
    expect(find.byKey(_bar), findsNothing);
    tester.view.resetViewInsets();
    await tester.pump();
    expect(find.byKey(_bar), findsOneWidget);

    await _native(tester, 'show', {'radarLeft': 0, 'radarBottom': 900});
    expect(find.byKey(_bar), findsNothing);
    expect(tester.takeException(), isNull);
  });
}
