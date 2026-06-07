import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:crazy_dice/main.dart';
import 'package:crazy_dice/roll_sound_controller.dart';

void main() {
  testWidgets('dice launcher rolls to a new face on tap', (
    WidgetTester tester,
  ) async {
    await tester.pumpWidget(
      const DiceLauncherApp(rollSoundController: SilentRollSoundController()),
    );

    expect(find.text('Crazy Dice'), findsOneWidget);
    expect(find.text('Tap anywhere to roll'), findsOneWidget);
    expect(find.byKey(const ValueKey('dice-face-1')), findsOneWidget);

    await tester.tap(find.byKey(const ValueKey('roll-surface')));
    await tester.pump();

    expect(find.text('Rolling...'), findsOneWidget);

    await tester.pumpAndSettle();

    expect(find.text('Tap anywhere to roll'), findsOneWidget);
    expect(find.byKey(const ValueKey('dice-face-1')), findsNothing);
  });
}
