import Flutter
import AVFoundation
import UIKit

private final class RollSoundPlugin: NSObject, FlutterPlugin {
  private var player: AVAudioPlayer?

  static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(
      name: "crazy_dice/roll_sound",
      binaryMessenger: registrar.messenger()
    )

    let instance = RollSoundPlugin()
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "playRollSound":
      guard
        let arguments = call.arguments as? [String: Any],
        let assetPath = arguments["assetPath"] as? String
      else {
        result(
          FlutterError(code: "INVALID_ARGS", message: "assetPath is required.", details: nil)
        )
        return
      }

      playSound(assetPath: assetPath, result: result)
    case "stopRollSound":
      player?.stop()
      player = nil
      result(nil)
    default:
      result(FlutterMethodNotImplemented)
    }
  }

  private func playSound(assetPath: String, result: @escaping FlutterResult) {
    let lookupKey = FlutterDartProject.lookupKey(forAsset: assetPath)

    guard let assetUrl = Bundle.main.url(forResource: lookupKey, withExtension: nil) else {
      result(
        FlutterError(code: "ASSET_NOT_FOUND", message: "Unable to load \(assetPath).", details: nil)
      )
      return
    }

    do {
      player?.stop()
      player = try AVAudioPlayer(contentsOf: assetUrl)
      player?.prepareToPlay()
      player?.play()
      result(nil)
    } catch {
      result(
        FlutterError(
          code: "PLAYBACK_FAILED",
          message: "Unable to play \(assetPath).",
          details: error.localizedDescription
        )
      )
    }
  }
}

@main
@objc class AppDelegate: FlutterAppDelegate, FlutterImplicitEngineDelegate {
  override func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    let didFinish = super.application(application, didFinishLaunchingWithOptions: launchOptions)

    if let registrar = self.registrar(forPlugin: "RollSoundPlugin") {
      RollSoundPlugin.register(with: registrar)
    }

    return didFinish
  }

  func didInitializeImplicitFlutterEngine(_ engineBridge: FlutterImplicitEngineBridge) {
    GeneratedPluginRegistrant.register(with: engineBridge.pluginRegistry)
  }
}
