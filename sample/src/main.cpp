#include "kutie/runtime.hpp"

#include "kutie/platform_services.hpp"

#include <Windows.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <ctime>
#include <thread>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    kutie::Runtime::Config config;
    config.main_window.title = "Kutie Demo";
    config.main_window.width = 960;
    config.main_window.height = 720;
    config.main_window.center = true;
    config.main_window.devtools = true;
    config.main_window.frame = true;

    kutie::Runtime app(config);
    auto& ipc = app.ipc();

    ipc.RegisterHandler("greet", [](const nlohmann::json& payload) -> nlohmann::json {
        const std::string name = payload.value("name", "World");
        return nlohmann::json{
            {"message", "Hello, " + name + "!"},
            {"framework", "Kutie"},
        };
    });

    ipc.RegisterHandler("math.add", [](const nlohmann::json& payload) -> nlohmann::json {
        const double a = payload.value("a", 0.0);
        const double b = payload.value("b", 0.0);
        return nlohmann::json{{"result", a + b}};
    });

    ipc.RegisterHandler("system.info", [](const nlohmann::json&) -> nlohmann::json {
        SYSTEM_INFO info{};
        GetNativeSystemInfo(&info);

        MEMORYSTATUSEX memory{};
        memory.dwLength = sizeof(memory);
        GlobalMemoryStatusEx(&memory);

        return nlohmann::json{
            {"cpu_cores", info.dwNumberOfProcessors},
            {"memory_mb", memory.ullTotalPhys / (1024 * 1024)},
            {"platform", "windows"},
            {"backend", "WebView2"},
        };
    });

    ipc.RegisterHandler("dialog.open", [](const nlohmann::json& payload) -> nlohmann::json {
        kutie::PlatformServices::OpenFileOptions options;
        options.title = payload.value("title", "Open File");
        const auto files = kutie::PlatformServices::OpenFiles(nullptr, options);
        return nlohmann::json{{"files", files}, {"cancelled", files.empty()}};
    });

    ipc.RegisterHandler("dialog.save", [](const nlohmann::json& payload) -> nlohmann::json {
        kutie::PlatformServices::SaveFileOptions options;
        options.title = payload.value("title", "Save File");
        options.default_name = payload.value("default_name", "untitled.txt");
        const auto path = kutie::PlatformServices::SaveFile(nullptr, options);
        return nlohmann::json{{"path", path.value_or("")}, {"cancelled", !path.has_value()}};
    });

    ipc.RegisterHandler("dialog.folder", [](const nlohmann::json& payload) -> nlohmann::json {
        const auto folder = kutie::PlatformServices::PickFolder(nullptr, payload.value("title", "Pick Folder"));
        return nlohmann::json{{"path", folder.value_or("")}, {"cancelled", !folder.has_value()}};
    });

    ipc.RegisterHandler("clipboard.read", [](const nlohmann::json&) -> nlohmann::json {
        const auto text = kutie::PlatformServices::ReadClipboardText();
        return nlohmann::json{{"text", text.value_or("")}, {"has_text", text.has_value()}};
    });

    ipc.RegisterHandler("clipboard.write", [](const nlohmann::json& payload) -> nlohmann::json {
        const bool ok = kutie::PlatformServices::WriteClipboardText(payload.value("text", ""));
        return nlohmann::json{{"ok", ok}};
    });

    static std::atomic<bool> heartbeat_running{true};
    app.OnReady([&app](kutie::Runtime&) {
        if (kutie::BrowserWindow::GetAll().empty()) {
            return;
        }
        kutie::BrowserWindow::GetAll().front()->OnClose([]() {
            heartbeat_running = false;
            return true;
        });

        std::thread([&app]() {
            int tick = 0;
            while (heartbeat_running) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                if (!heartbeat_running) {
                    break;
                }
                ++tick;
                app.ipc().Broadcast("heartbeat", {
                    {"tick", tick},
                    {"timestamp", std::time(nullptr)},
                });
            }
        }).detach();
    });

    return app.Run();
}
