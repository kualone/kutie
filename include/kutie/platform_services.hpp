#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kutie {

class PlatformServices {
public:
    struct FileFilter {
        std::string label;
        std::string pattern;
    };

    struct OpenFileOptions {
        std::string title = "Open File";
        std::vector<FileFilter> filters = {{"All Files", "*.*"}};
        bool multi_select = false;
    };

    struct SaveFileOptions {
        std::string title = "Save File";
        std::vector<FileFilter> filters = {{"All Files", "*.*"}};
        std::string default_name;
    };

    static std::vector<std::string> OpenFiles(void* parent_window, const OpenFileOptions& options);
    static std::optional<std::string> SaveFile(void* parent_window, const SaveFileOptions& options);
    static std::optional<std::string> PickFolder(void* parent_window, const std::string& title);

    static std::optional<std::string> ReadClipboardText();
    static bool WriteClipboardText(const std::string& text);
    static void ClearClipboard();

    static void ShowInfo(void* parent_window, const std::string& title, const std::string& message);
    static bool AskConfirm(void* parent_window, const std::string& title, const std::string& message);
};

} // namespace kutie
