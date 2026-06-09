#include "my_application.h"

#include <alsa/asoundlib.h>
#include <flutter_linux/flutter_linux.h>
#include <limits.h>
#include <unistd.h>

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "flutter/generated_plugin_registrant.h"

namespace {

constexpr char kRollSoundChannelName[] = "crazy_dice/roll_sound";

uint16_t read_uint16_le(std::istream& stream) {
  unsigned char bytes[2];
  stream.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
  return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

uint32_t read_uint32_le(std::istream& stream) {
  unsigned char bytes[4];
  stream.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
  return static_cast<uint32_t>(bytes[0] | (bytes[1] << 8) |
                               (bytes[2] << 16) | (bytes[3] << 24));
}

struct WavData {
  snd_pcm_format_t format;
  uint16_t channel_count;
  uint32_t sample_rate;
  size_t frame_size;
  std::vector<char> pcm_bytes;
};

bool load_wav_file(const std::string& path, WavData* wav_data) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return false;
  }

  char riff_header[4];
  stream.read(riff_header, sizeof(riff_header));
  if (stream.gcount() != sizeof(riff_header) ||
      std::string(riff_header, sizeof(riff_header)) != "RIFF") {
    return false;
  }

  stream.seekg(4, std::ios::cur);

  char wave_header[4];
  stream.read(wave_header, sizeof(wave_header));
  if (stream.gcount() != sizeof(wave_header) ||
      std::string(wave_header, sizeof(wave_header)) != "WAVE") {
    return false;
  }

  bool has_format_chunk = false;
  bool has_data_chunk = false;
  uint16_t audio_format = 0;
  uint16_t channel_count = 0;
  uint32_t sample_rate = 0;
  uint16_t bits_per_sample = 0;
  std::vector<char> pcm_bytes;

  while (stream.good() && (!has_format_chunk || !has_data_chunk)) {
    char chunk_id[4];
    stream.read(chunk_id, sizeof(chunk_id));
    if (stream.gcount() != sizeof(chunk_id)) {
      break;
    }

    const uint32_t chunk_size = read_uint32_le(stream);
    const std::string chunk_name(chunk_id, sizeof(chunk_id));

    if (chunk_name == "fmt ") {
      audio_format = read_uint16_le(stream);
      channel_count = read_uint16_le(stream);
      sample_rate = read_uint32_le(stream);
      stream.seekg(6, std::ios::cur);
      bits_per_sample = read_uint16_le(stream);

      if (chunk_size > 16) {
        stream.seekg(chunk_size - 16, std::ios::cur);
      }

      has_format_chunk = true;
    } else if (chunk_name == "data") {
      pcm_bytes.resize(chunk_size);
      stream.read(pcm_bytes.data(), static_cast<std::streamsize>(chunk_size));
      has_data_chunk = true;
    } else {
      stream.seekg(chunk_size, std::ios::cur);
    }

    if (chunk_size % 2 == 1) {
      stream.seekg(1, std::ios::cur);
    }
  }

  if (!has_format_chunk || !has_data_chunk || audio_format != 1 ||
      channel_count == 0) {
    return false;
  }

  snd_pcm_format_t pcm_format = SND_PCM_FORMAT_UNKNOWN;
  switch (bits_per_sample) {
    case 8:
      pcm_format = SND_PCM_FORMAT_U8;
      break;
    case 16:
      pcm_format = SND_PCM_FORMAT_S16_LE;
      break;
    case 24:
      pcm_format = SND_PCM_FORMAT_S24_3LE;
      break;
    case 32:
      pcm_format = SND_PCM_FORMAT_S32_LE;
      break;
    default:
      return false;
  }

  wav_data->format = pcm_format;
  wav_data->channel_count = channel_count;
  wav_data->sample_rate = sample_rate;
  wav_data->frame_size = channel_count * (bits_per_sample / 8);
  wav_data->pcm_bytes = std::move(pcm_bytes);
  return true;
}

std::string get_executable_directory() {
  char executable_path[PATH_MAX];
  const ssize_t executable_length =
      readlink("/proc/self/exe", executable_path, sizeof(executable_path) - 1);
  if (executable_length <= 0) {
    return std::string();
  }

  executable_path[executable_length] = '\0';
  std::string executable(executable_path);
  const size_t separator = executable.find_last_of('/');
  return executable.substr(0, separator);
}

std::string get_data_path(const char* relative_path) {
  const std::string executable_directory = get_executable_directory();
  if (executable_directory.empty()) {
    return std::string();
  }

  return executable_directory + "/data/" + relative_path;
}

std::string get_asset_path(const gchar* asset_path) {
  const std::string data_path = get_data_path("flutter_assets");
  if (data_path.empty()) {
    return std::string();
  }

  return data_path + "/" + asset_path;
}

class CrazyDiceSoundPlayer {
 public:
  CrazyDiceSoundPlayer() = default;

  ~CrazyDiceSoundPlayer() { Stop(); }

  void Play(const std::string& asset_path) {
    Stop();

    cancel_playback_.store(false);
    playback_thread_ = std::thread([this, asset_path]() {
      PlayWavFile(get_asset_path(asset_path.c_str()));
    });
  }

  void Stop() {
    cancel_playback_.store(true);

    snd_pcm_t* pcm_handle = nullptr;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      pcm_handle = pcm_handle_;
    }

    if (pcm_handle != nullptr) {
      snd_pcm_drop(pcm_handle);
    }

    if (playback_thread_.joinable()) {
      playback_thread_.join();
    }
  }

 private:
  void PlayWavFile(const std::string& path) {
    WavData wav_data;
    if (!load_wav_file(path, &wav_data)) {
      return;
    }

    snd_pcm_t* pcm_handle = nullptr;
    if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      pcm_handle_ = pcm_handle;
    }

    const int set_params_result = snd_pcm_set_params(
        pcm_handle, wav_data.format, SND_PCM_ACCESS_RW_INTERLEAVED,
        wav_data.channel_count, wav_data.sample_rate, 1, 500000);
    if (set_params_result < 0) {
      clear_handle(pcm_handle);
      snd_pcm_close(pcm_handle);
      return;
    }

    const char* buffer = wav_data.pcm_bytes.data();
    snd_pcm_uframes_t frames_remaining =
        wav_data.pcm_bytes.size() / wav_data.frame_size;

    while (frames_remaining > 0 && !cancel_playback_.load()) {
      const snd_pcm_sframes_t written_frames =
          snd_pcm_writei(pcm_handle, buffer, frames_remaining);
      if (written_frames == -EPIPE) {
        snd_pcm_prepare(pcm_handle);
        continue;
      }
      if (written_frames < 0) {
        const int recovered = snd_pcm_recover(
            pcm_handle, static_cast<int>(written_frames), 0);
        if (recovered < 0) {
          break;
        }
        continue;
      }

      buffer += written_frames * wav_data.frame_size;
      frames_remaining -= written_frames;
    }

    if (!cancel_playback_.load()) {
      snd_pcm_drain(pcm_handle);
    }

    clear_handle(pcm_handle);
    snd_pcm_close(pcm_handle);
  }

  void clear_handle(snd_pcm_t* pcm_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pcm_handle_ == pcm_handle) {
      pcm_handle_ = nullptr;
    }
  }

  std::atomic<bool> cancel_playback_{false};
  std::mutex mutex_;
  std::thread playback_thread_;
  snd_pcm_t* pcm_handle_ = nullptr;
};

}  // namespace

struct _MyApplication {
  GtkApplication parent_instance;
  char** dart_entrypoint_arguments;
  FlView* view;
  FlMethodChannel* roll_sound_channel;
  CrazyDiceSoundPlayer* sound_player;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION)

static FlMethodResponse* handle_play_roll_sound(MyApplication* self,
                                                FlMethodCall* method_call) {
  FlValue* arguments = fl_method_call_get_args(method_call);
  FlValue* asset_path_value = fl_value_lookup_string(arguments, "assetPath");
  if (asset_path_value == nullptr ||
      fl_value_get_type(asset_path_value) != FL_VALUE_TYPE_STRING) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "INVALID_ARGS", "assetPath is required.", nullptr));
  }

  self->sound_player->Play(fl_value_get_string(asset_path_value));
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

static void roll_sound_method_call_cb(FlMethodChannel* channel,
                                      FlMethodCall* method_call,
                                      gpointer user_data) {
  MyApplication* self = MY_APPLICATION(user_data);

  g_autoptr(FlMethodResponse) response = nullptr;
  const gchar* method_name = fl_method_call_get_name(method_call);

  if (strcmp(method_name, "playRollSound") == 0) {
    response = handle_play_roll_sound(self, method_call);
  } else if (strcmp(method_name, "stopRollSound") == 0) {
    self->sound_player->Stop();
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  g_autoptr(GError) error = nullptr;
  if (!fl_method_call_respond(method_call, response, &error)) {
    g_warning("Failed to send roll sound response: %s", error->message);
  }
}

static void create_channels(MyApplication* self) {
  FlEngine* engine = fl_view_get_engine(self->view);
  FlBinaryMessenger* messenger = fl_engine_get_binary_messenger(engine);
  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();

  self->roll_sound_channel = fl_method_channel_new(
      messenger, kRollSoundChannelName, FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(
      self->roll_sound_channel, roll_sound_method_call_cb, self, nullptr);
}

// Called when first Flutter frame received.
static void first_frame_cb(MyApplication* self, FlView* view) {
  gtk_widget_show(gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

// Implements GApplication::activate.
static void my_application_activate(GApplication* application) {
  MyApplication* self = MY_APPLICATION(application);
  GtkWindow* window =
      GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));
  const std::string app_icon_path = get_data_path("app_icon.png");
  if (!app_icon_path.empty()) {
    g_autoptr(GError) icon_error = nullptr;
    if (!gtk_window_set_icon_from_file(window, app_icon_path.c_str(),
                                       &icon_error)) {
      g_warning("Failed to set application icon: %s", icon_error->message);
    }
  }

  gboolean use_header_bar = TRUE;
#ifdef GDK_WINDOWING_X11
  GdkScreen* screen = gtk_window_get_screen(window);
  if (GDK_IS_X11_SCREEN(screen)) {
    const gchar* wm_name = gdk_x11_screen_get_window_manager_name(screen);
    if (g_strcmp0(wm_name, "GNOME Shell") != 0) {
      use_header_bar = FALSE;
    }
  }
#endif
  if (use_header_bar) {
    GtkHeaderBar* header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
    gtk_widget_show(GTK_WIDGET(header_bar));
    gtk_header_bar_set_title(header_bar, "Crazy Dice");
    gtk_header_bar_set_show_close_button(header_bar, TRUE);
    gtk_window_set_titlebar(window, GTK_WIDGET(header_bar));
  } else {
    gtk_window_set_title(window, "Crazy Dice");
  }

  gtk_window_set_default_size(window, 1280, 720);

  g_autoptr(FlDartProject) project = fl_dart_project_new();
  fl_dart_project_set_dart_entrypoint_arguments(
      project, self->dart_entrypoint_arguments);

  self->view = fl_view_new(project);
  GdkRGBA background_color;
  gdk_rgba_parse(&background_color, "#000000");
  fl_view_set_background_color(self->view, &background_color);
  gtk_widget_show(GTK_WIDGET(self->view));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(self->view));

  g_signal_connect_swapped(self->view, "first-frame",
                           G_CALLBACK(first_frame_cb), self);
  gtk_widget_realize(GTK_WIDGET(self->view));

  fl_register_plugins(FL_PLUGIN_REGISTRY(self->view));
  create_channels(self);

  gtk_widget_grab_focus(GTK_WIDGET(self->view));
}

// Implements GApplication::local_command_line.
static gboolean my_application_local_command_line(GApplication* application,
                                                  gchar*** arguments,
                                                  int* exit_status) {
  MyApplication* self = MY_APPLICATION(application);
  self->dart_entrypoint_arguments = g_strdupv(*arguments + 1);

  g_autoptr(GError) error = nullptr;
  if (!g_application_register(application, nullptr, &error)) {
    g_warning("Failed to register: %s", error->message);
    *exit_status = 1;
    return TRUE;
  }

  g_application_activate(application);
  *exit_status = 0;

  return TRUE;
}

// Implements GApplication::startup.
static void my_application_startup(GApplication* application) {
  G_APPLICATION_CLASS(my_application_parent_class)->startup(application);
}

// Implements GApplication::shutdown.
static void my_application_shutdown(GApplication* application) {
  MyApplication* self = MY_APPLICATION(application);
  if (self->sound_player != nullptr) {
    self->sound_player->Stop();
  }
  G_APPLICATION_CLASS(my_application_parent_class)->shutdown(application);
}

// Implements GObject::dispose.
static void my_application_dispose(GObject* object) {
  MyApplication* self = MY_APPLICATION(object);
  g_clear_pointer(&self->dart_entrypoint_arguments, g_strfreev);
  g_clear_object(&self->roll_sound_channel);
  delete self->sound_player;
  self->sound_player = nullptr;
  G_OBJECT_CLASS(my_application_parent_class)->dispose(object);
}

static void my_application_class_init(MyApplicationClass* klass) {
  G_APPLICATION_CLASS(klass)->activate = my_application_activate;
  G_APPLICATION_CLASS(klass)->local_command_line =
      my_application_local_command_line;
  G_APPLICATION_CLASS(klass)->startup = my_application_startup;
  G_APPLICATION_CLASS(klass)->shutdown = my_application_shutdown;
  G_OBJECT_CLASS(klass)->dispose = my_application_dispose;
}

static void my_application_init(MyApplication* self) {
  self->view = nullptr;
  self->roll_sound_channel = nullptr;
  self->sound_player = new CrazyDiceSoundPlayer();
}

MyApplication* my_application_new() {
  g_set_prgname(APPLICATION_ID);

  return MY_APPLICATION(g_object_new(my_application_get_type(),
                                     "application-id", APPLICATION_ID, "flags",
                                     G_APPLICATION_NON_UNIQUE, nullptr));
}
