import 'dart:math';

import 'package:crazy_dice/roll_sound_controller.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);

  runApp(
    DiceLauncherApp(rollSoundController: createDefaultRollSoundController()),
  );
}

class DiceLauncherApp extends StatelessWidget {
  const DiceLauncherApp({
    super.key,
    this.rollSoundController = const SilentRollSoundController(),
  });

  final RollSoundController rollSoundController;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Crazy Dice',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(useMaterial3: true, brightness: Brightness.dark),
      home: DiceLauncherScreen(rollSoundController: rollSoundController),
    );
  }
}

class DiceLauncherScreen extends StatefulWidget {
  const DiceLauncherScreen({super.key, required this.rollSoundController});

  final RollSoundController rollSoundController;

  @override
  State<DiceLauncherScreen> createState() => _DiceLauncherScreenState();
}

class _DiceLauncherScreenState extends State<DiceLauncherScreen>
    with SingleTickerProviderStateMixin {
  final Random _random = Random();
  late final AnimationController _rollController;

  static const List<_DiceFaceStyle> _faceStyles = [
    _DiceFaceStyle(background: Color(0xFF9B2226)),
    _DiceFaceStyle(background: Color(0xFF005F73)),
    _DiceFaceStyle(background: Color(0xFF0A9396)),
    _DiceFaceStyle(background: Color(0xFFBB3E03)),
    _DiceFaceStyle(background: Color(0xFF6A4C93)),
    _DiceFaceStyle(background: Color(0xFF386641)),
  ];

  int _currentFace = 1;
  int _startFace = 1;
  int _targetFace = 1;
  int _shuffleSteps = 0;
  int _lastShuffleStep = -1;
  bool _isRolling = false;

  @override
  void initState() {
    super.initState();
    _rollController =
        AnimationController(
            vsync: this,
            duration: const Duration(milliseconds: 1200),
          )
          ..addListener(_handleRollProgress)
          ..addStatusListener(_handleRollStatusChange);
  }

  @override
  void dispose() {
    widget.rollSoundController.stop();
    _rollController
      ..removeListener(_handleRollProgress)
      ..removeStatusListener(_handleRollStatusChange)
      ..dispose();
    super.dispose();
  }

  void _handleRollProgress() {
    final step = (_rollController.value * _shuffleSteps).floor();

    if (step == _lastShuffleStep || step >= _shuffleSteps) {
      return;
    }

    _lastShuffleStep = step;
    final previewFace = ((_startFace - 1 + step + 1) % 6) + 1;

    if (previewFace != _currentFace) {
      setState(() {
        _currentFace = previewFace;
      });
    }
  }

  void _handleRollStatusChange(AnimationStatus status) {
    if (status != AnimationStatus.completed) {
      return;
    }

    setState(() {
      _currentFace = _targetFace;
      _isRolling = false;
    });

    widget.rollSoundController.stop();
  }

  void _launchDice() {
    if (_isRolling) {
      return;
    }

    final nextFace = _nextFaceExcluding(_currentFace);

    setState(() {
      _isRolling = true;
      _startFace = _currentFace;
      _targetFace = nextFace;
      _shuffleSteps = 18 + _random.nextInt(6);
      _lastShuffleStep = -1;
      _rollController.duration = Duration(
        milliseconds: 950 + _random.nextInt(350),
      );
    });

    widget.rollSoundController.start();
    _rollController.forward(from: 0);
  }

  int _nextFaceExcluding(int currentFace) {
    final offset = 1 + _random.nextInt(5);
    return ((currentFace - 1 + offset) % 6) + 1;
  }

  @override
  Widget build(BuildContext context) {
    final style = _faceStyles[_currentFace - 1];
    final rollValue = _rollController.value;
    final rotation = sin(rollValue * pi * 7) * 0.2;
    final scale = 1 + sin(rollValue * pi) * 0.08;

    return Scaffold(
      body: GestureDetector(
        key: const ValueKey('roll-surface'),
        behavior: HitTestBehavior.opaque,
        onTap: _launchDice,
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 260),
          curve: Curves.easeOutCubic,
          color: style.background,
          child: SafeArea(
            child: LayoutBuilder(
              builder: (context, constraints) {
                final diceSize =
                    min(constraints.maxWidth, constraints.maxHeight) * 0.68;

                return Stack(
                  children: [
                    Positioned(
                      top: 20,
                      left: 20,
                      child: _InfoChip(
                        label: 'Crazy Dice',
                        value: _isRolling ? 'Rolling' : 'Ready',
                      ),
                    ),
                    Positioned(
                      top: 20,
                      right: 20,
                      child: _InfoChip(label: 'Face', value: '$_currentFace'),
                    ),
                    Center(
                      child: Transform.scale(
                        scale: scale,
                        child: Transform.rotate(
                          angle: rotation,
                          child: AnimatedSwitcher(
                            duration: const Duration(milliseconds: 180),
                            switchInCurve: Curves.easeOutBack,
                            switchOutCurve: Curves.easeIn,
                            transitionBuilder: (child, animation) {
                              return FadeTransition(
                                opacity: animation,
                                child: ScaleTransition(
                                  scale: Tween<double>(
                                    begin: 0.85,
                                    end: 1,
                                  ).animate(animation),
                                  child: child,
                                ),
                              );
                            },
                            child: _DiceFace(
                              key: ValueKey('dice-face-$_currentFace'),
                              face: _currentFace,
                              size: diceSize,
                            ),
                          ),
                        ),
                      ),
                    ),
                    Positioned(
                      left: 20,
                      right: 20,
                      bottom: 24,
                      child: Text(
                        _isRolling ? 'Rolling...' : 'Tap anywhere to roll',
                        textAlign: TextAlign.center,
                        style: Theme.of(context).textTheme.titleMedium
                            ?.copyWith(
                              color: Colors.white,
                              fontWeight: FontWeight.w700,
                              letterSpacing: 0.5,
                            ),
                      ),
                    ),
                  ],
                );
              },
            ),
          ),
        ),
      ),
    );
  }
}

class _DiceFace extends StatelessWidget {
  const _DiceFace({required super.key, required this.face, required this.size});

  final int face;
  final double size;

  static const double _pipOffset = 0.56;
  static const Map<int, List<Alignment>> _pipLayout = {
    1: [Alignment.center],
    2: [Alignment(-_pipOffset, -_pipOffset), Alignment(_pipOffset, _pipOffset)],
    3: [
      Alignment(-_pipOffset, -_pipOffset),
      Alignment.center,
      Alignment(_pipOffset, _pipOffset),
    ],
    4: [
      Alignment(-_pipOffset, -_pipOffset),
      Alignment(_pipOffset, -_pipOffset),
      Alignment(-_pipOffset, _pipOffset),
      Alignment(_pipOffset, _pipOffset),
    ],
    5: [
      Alignment(-_pipOffset, -_pipOffset),
      Alignment(_pipOffset, -_pipOffset),
      Alignment.center,
      Alignment(-_pipOffset, _pipOffset),
      Alignment(_pipOffset, _pipOffset),
    ],
    6: [
      Alignment(-_pipOffset, -_pipOffset),
      Alignment(-_pipOffset, 0),
      Alignment(-_pipOffset, _pipOffset),
      Alignment(_pipOffset, -_pipOffset),
      Alignment(_pipOffset, 0),
      Alignment(_pipOffset, _pipOffset),
    ],
  };

  @override
  Widget build(BuildContext context) {
    final pipSize = size * 0.11;

    return Semantics(
      label: 'Dice face $face',
      child: Container(
        width: size,
        height: size,
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(size * 0.14),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: 0.18),
              blurRadius: 36,
              offset: const Offset(0, 20),
            ),
          ],
        ),
        child: Stack(
          children: [
            for (final alignment in _pipLayout[face]!)
              Align(
                alignment: alignment,
                child: Container(
                  width: pipSize,
                  height: pipSize,
                  decoration: const BoxDecoration(
                    color: Color(0xFF111111),
                    shape: BoxShape.circle,
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}

class _InfoChip extends StatelessWidget {
  const _InfoChip({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    return DecoratedBox(
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.16),
        borderRadius: BorderRadius.circular(999),
        border: Border.all(color: Colors.white.withValues(alpha: 0.22)),
      ),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.center,
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              label,
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.labelLarge?.copyWith(
                color: const Color(0xFFD7E3FC),
                fontWeight: FontWeight.w500,
                height: 1.2,
              ),
            ),
            Text(
              value,
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                color: Colors.white,
                fontWeight: FontWeight.w800,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _DiceFaceStyle {
  const _DiceFaceStyle({required this.background});

  final Color background;
}
