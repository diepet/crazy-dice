# Crazy Dice

`Crazy Dice` is a small Flutter app that turns the entire screen into a dice roller. Tap anywhere to launch a roll.

This is just an experiment to generate a multi-platform app through AI and release the app to one or more official stores.

## Features

- Full-screen tap-to-roll interaction
- Animated dice face transitions with scale and rotation
- Randomized target face that always changes from the current one
- Native sound playback on supported desktop and mobile platforms
- Asset-based roll sounds bundled in `assets/audio/`

## Platform Sound Behavior

The app uses a shared Dart interface with platform-specific implementations behind a `MethodChannel` named `crazy_dice/roll_sound`.

- Android: `MediaPlayer`
- iOS and macOS: `AVAudioPlayer`
- Linux: ALSA playback
- Windows: `PlaySoundW`
- Web or unsupported platforms: Flutter system click fallback

This keeps audio playback native without introducing a Flutter audio plugin dependency.

## Tested Platforms

- Tested: Android and Web
- Present in the repository but not tested: iOS, macOS, Linux, and Windows

Native runner code exists for the untested platforms, but those targets have not been verified in this project.

## Project Layout

- `lib/main.dart`: app entry point and dice UI
- `lib/roll_sound_controller.dart`: public sound controller factory
- `lib/src/`: base and platform-aware sound controller implementations
- `assets/audio/`: bundled dice roll sound assets
- `test/widget_test.dart`: widget coverage for the roll interaction

## Requirements

- Flutter SDK compatible with Dart `^3.12.1`

## Run Locally

From the project root:

```bash
flutter pub get --offline
flutter run
```

If dependency downloads are unreliable in the local environment, prefer the `--offline` and `--no-pub` variants already used in this repository.

## Development Commands

```bash
flutter analyze --no-pub
flutter test --no-pub
flutter build linux --debug --no-pub
flutter build apk --debug --no-pub
```

## Notes

- The codebase was generated with Codex and OpenAI GPT models.
- The app starts in immersive sticky mode to minimize visible system UI.
- Audio behavior must stay aligned across Dart and native runner implementations when the sound contract changes.
- Product name: `Crazy Dice`
- Package name: `crazy_dice`
- App identifier: `it.diepet.crazydice`
