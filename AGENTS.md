# Repository Guidelines

## Project Structure & Module Organization
This repository contains the `Crazy Dice` Flutter application. App code lives in `lib/`, with the main entry point in `lib/main.dart`, sound-controller abstractions in `lib/roll_sound_controller.dart`, and platform-aware sound implementations under `lib/src/`. Audio assets live in `assets/audio/` and are bundled through `pubspec.yaml`. Widget tests live in `test/`. Platform wrappers and native integrations live under `android/`, `ios/`, `macos/`, `linux/`, `windows/`, and `web/`. Generated output under `build/`, `.dart_tool/`, and platform ephemeral/cache directories should be treated as disposable.

## Build, Test, and Development Commands
Run commands from the project root:

- `flutter pub get --offline` refreshes the lockfile and package graph without downloading anything new.
- `flutter run` launches the app on the selected device or desktop target.
- `flutter analyze --no-pub` runs static analysis without triggering dependency resolution.
- `flutter test --no-pub` runs the widget test suite.
- `flutter build linux --debug --no-pub` builds the Linux desktop app bundle.
- `flutter build apk --debug --no-pub` builds the Android debug APK.

Use the `--no-pub` or `--offline` variants when dependency downloads are unreliable in the local environment.

## Coding Style & Naming Conventions
Follow standard Dart style: 2-space indentation, trailing commas where they improve formatter output, `UpperCamelCase` for types, and `lowerCamelCase` for members. Keep Dart filenames in `lower_snake_case.dart`. Format Dart sources with `dart format`. Keep UI code composable instead of growing `main.dart` unnecessarily. Preserve the current app branding and identifiers:

- Product name: `Crazy Dice`
- Dart package: `crazy_dice`
- Android/iOS/macOS/Linux app identifier: `it.diepet.crazydice`

## Native Platform Notes
The app uses native sound playback per platform instead of a Flutter audio package:

- Android: `MediaPlayer` in `android/app/src/main/kotlin/it/diepet/crazydice/MainActivity.kt`
- iOS/macOS: `AVAudioPlayer` in the platform `AppDelegate.swift`
- Windows: `PlaySoundW` in `windows/runner/flutter_window.cpp`
- Linux: ALSA playback in `linux/runner/my_application.cc`

When editing sound behavior, keep the Dart `MethodChannel` contract aligned with all platform implementations.

## Testing Guidelines
Add or update tests in `test/` when app behavior changes. Prefer focused widget tests for interaction and state changes. Run `flutter analyze --no-pub` and `flutter test --no-pub` before finishing work. If native runner code changes on Linux or Android, also validate with a platform build when possible.

## Commit & Pull Request Guidelines
This checkout does not contain a `.git/` directory, so local history is unavailable. Use short imperative commit messages with a clear scope, such as `android: rename package to it.diepet.crazydice` or `lib: randomize native roll sounds`. PRs should summarize user-visible changes, list affected platforms, and note any identifier or native-runner changes explicitly.

## Generated Files & Configuration
Do not hand-edit generated files inside `build/`, `.dart_tool/`, `ios/Flutter/`, `macos/Flutter/`, `linux/flutter/`, or `windows/flutter/` unless debugging the toolchain itself. Keep secrets and machine-local paths out of committed files, and review platform config files before sharing patches externally.
