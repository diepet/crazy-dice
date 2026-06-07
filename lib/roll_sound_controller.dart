import 'src/roll_sound_controller_base.dart';
import 'src/roll_sound_controller_factory_stub.dart'
    if (dart.library.io) 'src/roll_sound_controller_factory_io.dart'
    as roll_sound_factory;

export 'src/roll_sound_controller_base.dart';

RollSoundController createDefaultRollSoundController() {
  return roll_sound_factory.createDefaultRollSoundController();
}
