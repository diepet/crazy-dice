import 'dart:async';
import 'dart:math';

import 'package:flutter/services.dart';

import 'roll_sound_controller_base.dart';

const _rollSoundChannel = MethodChannel('crazy_dice/roll_sound');

RollSoundController createDefaultRollSoundController() {
  return NativeRollSoundController(
    assetPaths: const [
      'assets/audio/dice_sound_1.wav',
      'assets/audio/dice_sound_2.wav',
      'assets/audio/dice_sound_3.wav',
      'assets/audio/dice_sound_4.wav',
    ],
  );
}

class NativeRollSoundController extends RollSoundController {
  NativeRollSoundController({required this.assetPaths});

  final List<String> assetPaths;
  final Random _random = Random();

  @override
  void start() {
    final assetPath = assetPaths[_random.nextInt(assetPaths.length)];
    unawaited(
      _rollSoundChannel.invokeMethod<void>('playRollSound', {
        'assetPath': assetPath,
      }),
    );
  }

  @override
  void stop() {
    unawaited(_rollSoundChannel.invokeMethod<void>('stopRollSound'));
  }
}
