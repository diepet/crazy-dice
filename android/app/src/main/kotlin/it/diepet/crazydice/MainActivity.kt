package it.diepet.crazydice

import android.media.AudioAttributes
import android.media.MediaPlayer
import io.flutter.FlutterInjector
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
    private var mediaPlayer: MediaPlayer? = null

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)

        MethodChannel(
            flutterEngine.dartExecutor.binaryMessenger,
            "crazy_dice/roll_sound",
        ).setMethodCallHandler { call, result ->
            when (call.method) {
                "playRollSound" -> {
                    val assetPath = call.argument<String>("assetPath")
                    if (assetPath == null) {
                        result.error("INVALID_ARGS", "assetPath is required.", null)
                    } else {
                        playRollSound(assetPath)
                        result.success(null)
                    }
                }

                "stopRollSound" -> {
                    stopRollSound()
                    result.success(null)
                }

                else -> result.notImplemented()
            }
        }
    }

    override fun onDestroy() {
        stopRollSound()
        super.onDestroy()
    }

    private fun playRollSound(assetPath: String) {
        stopRollSound()

        val lookupKey = FlutterInjector.instance().flutterLoader().getLookupKeyForAsset(assetPath)
        val assetDescriptor = assets.openFd(lookupKey)

        mediaPlayer = MediaPlayer().apply {
            setAudioAttributes(
                AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build(),
            )
            setDataSource(
                assetDescriptor.fileDescriptor,
                assetDescriptor.startOffset,
                assetDescriptor.length,
            )
            setOnCompletionListener {
                it.release()
                if (mediaPlayer === it) {
                    mediaPlayer = null
                }
            }
            setOnErrorListener { player, _, _ ->
                player.release()
                if (mediaPlayer === player) {
                    mediaPlayer = null
                }
                true
            }
            prepare()
            start()
        }

        assetDescriptor.close()
    }

    private fun stopRollSound() {
        mediaPlayer?.let { player ->
            if (player.isPlaying) {
                player.stop()
            }
            player.release()
        }
        mediaPlayer = null
    }
}
