#include "flutter_window.h"

#include <algorithm>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>
#include <mmsystem.h>
#include <optional>
#include <string>

#include "flutter/generated_plugin_registrant.h"

namespace {

std::wstring GetExecutableDirectory() {
  wchar_t executable_path[MAX_PATH];
  GetModuleFileNameW(nullptr, executable_path, MAX_PATH);

  std::wstring path(executable_path);
  const auto separator = path.find_last_of(L"\\/");
  return path.substr(0, separator);
}

std::wstring GetFlutterAssetPath(const std::string& asset_path) {
  std::wstring relative_path(asset_path.begin(), asset_path.end());
  std::replace(relative_path.begin(), relative_path.end(), L'/', L'\\');
  return GetExecutableDirectory() + L"\\data\\flutter_assets\\" + relative_path;
}

}  // namespace

FlutterWindow::FlutterWindow(const flutter::DartProject& project)
    : project_(project) {}

FlutterWindow::~FlutterWindow() {}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  // The size here must match the window dimensions to avoid unnecessary surface
  // creation / destruction in the startup path.
  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);
  // Ensure that basic setup of the controller was successful.
  if (!flutter_controller_->engine() || !flutter_controller_->view()) {
    return false;
  }
  RegisterPlugins(flutter_controller_->engine());

  flutter::MethodChannel<> channel(
      flutter_controller_->engine()->messenger(), "crazy_dice/roll_sound",
      &flutter::StandardMethodCodec::GetInstance());
  channel.SetMethodCallHandler(
      [](const flutter::MethodCall<>& call,
         std::unique_ptr<flutter::MethodResult<>> result) {
        if (call.method_name() == "playRollSound") {
          const auto* arguments =
              std::get_if<flutter::EncodableMap>(call.arguments());
          if (arguments == nullptr) {
            result->Error("INVALID_ARGS", "assetPath is required.");
            return;
          }

          const auto asset_path = arguments->find(
              flutter::EncodableValue(std::string("assetPath")));
          if (asset_path == arguments->end()) {
            result->Error("INVALID_ARGS", "assetPath is required.");
            return;
          }

          const auto* value = std::get_if<std::string>(&asset_path->second);
          if (value == nullptr) {
            result->Error("INVALID_ARGS", "assetPath must be a string.");
            return;
          }

          PlaySoundW(GetFlutterAssetPath(*value).c_str(), nullptr,
                     SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
          result->Success();
        } else if (call.method_name() == "stopRollSound") {
          PlaySoundW(nullptr, nullptr, 0);
          result->Success();
        } else {
          result->NotImplemented();
        }
      });

  SetChildContent(flutter_controller_->view()->GetNativeWindow());

  flutter_controller_->engine()->SetNextFrameCallback([&]() {
    this->Show();
  });

  // Flutter can complete the first frame before the "show window" callback is
  // registered. The following call ensures a frame is pending to ensure the
  // window is shown. It is a no-op if the first frame hasn't completed yet.
  flutter_controller_->ForceRedraw();

  return true;
}

void FlutterWindow::OnDestroy() {
  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }

  Win32Window::OnDestroy();
}

LRESULT
FlutterWindow::MessageHandler(HWND hwnd, UINT const message,
                              WPARAM const wparam,
                              LPARAM const lparam) noexcept {
  // Give Flutter, including plugins, an opportunity to handle window messages.
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam,
                                                      lparam);
    if (result) {
      return *result;
    }
  }

  switch (message) {
    case WM_FONTCHANGE:
      flutter_controller_->engine()->ReloadSystemFonts();
      break;
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}
