import 'package:flutter/services.dart';

abstract class RollSoundController {
  const RollSoundController();

  void start();

  void stop();
}

class SilentRollSoundController extends RollSoundController {
  const SilentRollSoundController();

  @override
  void start() {}

  @override
  void stop() {}
}

class SystemClickRollSoundController extends RollSoundController {
  const SystemClickRollSoundController();

  @override
  void start() {
    SystemSound.play(SystemSoundType.click);
  }

  @override
  void stop() {}
}
