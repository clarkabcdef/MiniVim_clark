#include "Editor.hpp"

#include <algorithm>
#include <cctype>
#include <exception>

namespace sjtu {

namespace {
// 匿名namespace,提供了只给当前文件使用的辅助函数

std::string Trim(std::string value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

// 判断是否为ASCII可打印字符,Tab由插入模式另外处理
bool IsPrintable(char value) { return value >= 0x20U && value < 0x7FU; }

} // namespace

Editor::Editor(const std::filesystem::path& path) : buffer_(path), terminal_() {}

void Editor::Run() {
    while (running_) {
        RefreshScreen();
        ProcessKey(terminal_.ReadKey());
    }
    terminal_.ClearScreen();
}

bool Editor::IsRunning() const noexcept { return running_; }

void Editor::RefreshScreen() {
    window_.Resize(terminal_.GetScreenSize());
    window_.EnsureCursorVisible(buffer_);

    RenderState state{mode_, command_, message_};
    terminal_.WriteOutput(renderer_.Render(buffer_, window_, state));
}

void Editor::ProcessKey(KeyEvent key) {
    if (key.IsControl('q')) {
        running_ = false;
        return;
    }
    
    switch (mode_) {
    case Mode::Insert:
        HandleInsert(key);
        return;
    case Mode::CommandLine:
        HandleCommandLine(key);
        return; 
    case Mode::Normal:
        message_.clear();
        Execute(normal_parser_.Feed(key));
        return;
    }   
}

void Editor::Execute(const EditorAction& action) {
    switch (action.kind_) {
    case ActionKind::None:
        return;
    case ActionKind::Move:
        if (action.motion_.has_value()) {
            window_.ApplyMotion(buffer_, action.motion_.value());
        }
        return;
    case ActionKind::InsertBefore:
        EnterInsert(window_.GetCursor());
        return;
    case ActionKind::InsertAfter: {
        Position position = window_.GetCursor();
        const std::string& line = buffer_.GetLineAt(position.row_);
        if (position.column_ < line.size()) {
            ++position.column_;
        }
        EnterInsert(position);
        return;
    }
    case ActionKind::EnterCommandLine:
        mode_ = Mode::CommandLine;
        command_.clear();
        message_.clear(); 
        return;
    }
}

void Editor::HandleInsert(KeyEvent key) {
    if (key.code_ == KeyCode::Escape) {
        LeaveInsert();
        return;
    }

    Position position = window_.GetCursor();

    if (key.code_ == KeyCode::Enter) {
        buffer_.SplitLine(position.row_, position.column_);
        position.row_++;
        position.column_ = 0;
        window_.SetCursor(buffer_, position, true);
        return;
    }
    
    if (key.code_ == KeyCode::Backspace) {
        if (position.column_ > 0) {
            buffer_.EraseCharacter(position.row_, position.column_ - 1);
            --position.column_;
            window_.SetCursor(buffer_, position, true);
        } else if (position.row_ > 0) {
            const std::size_t previous_row = position.row_ - 1;
            const std::size_t previous_line_length = buffer_.GetLineAt(previous_row).size();

            buffer_.JoinLine(previous_row);
            position.row_ = previous_row;
            position.column_ = previous_line_length;
            window_.SetCursor(buffer_, position, true);
        }
        return;
    }
    
    if (key.code_ == KeyCode::Character){
        const char value = key.value_;
        if (value == '\t' || IsPrintable(value)) {
            buffer_.InsertCharacter(position.row_, position.column_, value);
            ++position.column_;
            window_.SetCursor(buffer_, position, true);
        }
    }
}

void Editor::EnterInsert(Position position) {
    mode_ = Mode::Insert;
    message_.clear();
    window_.SetCursor(buffer_, position, true);
}

void Editor::LeaveInsert() {
    Position position = window_.GetCursor();
    if (position.column_ > 0) {
        --position.column_;
    }
    mode_ = Mode::Normal;
    window_.SetCursor(buffer_, position, false);
}

void Editor::HandleCommandLine(KeyEvent key) {
    if (key.code_ == KeyCode::Escape) {
        LeaveCommandLine();
        return;
    }
    if (key.code_ == KeyCode::Enter) {
        ExecuteCommandLine();
        return;
    }
    if (key.code_ == KeyCode::Backspace || key.code_ == KeyCode::Delete) {
        if (!command_.empty()) {
            command_.pop_back();
        }
        return;
    }
    if (key.code_ == KeyCode::Character && IsPrintable(key.value_)) {
        command_.push_back(key.value_);
    }
}

void Editor::ExecuteCommandLine() {
    const std::string trimmed_command = Trim(command_);
    if (trimmed_command.empty()) {
        LeaveCommandLine();
        return;
    }

    const std::size_t separator = trimmed_command.find_first_of(" \t");
    const std::string command_name = (separator == std::string::npos) ? trimmed_command : trimmed_command.substr(0, separator);
    const std::string command_argument = (separator == std::string::npos) ? "" : Trim(trimmed_command.substr(separator + 1));

    if (command_name == "w" || command_name == "wq") {
        bool success = false;
        if (command_argument.empty()) {
            success = SaveBuffer();
        } else {
            success = SaveBuffer(std::filesystem::path(command_argument));
        }

        if (success && command_name == "wq") {
            running_ = false;
        }
        LeaveCommandLine();
        return;
    } 
    
    if (command_name == "q" || command_name == "quit") {
        if (buffer_.IsModified()) {
            message_ = "File has changed. Use :wq to save and quit.";
            LeaveCommandLine(); 
        } else {
            running_ = false;   
        }
        return;
    } 
    
    if (command_name == "q!" || command_name == "quit!") {
        running_ = false;       
        return;
    } 

    message_ = "Unknown command: " + command_name;
    LeaveCommandLine(); 
}

void Editor::LeaveCommandLine() {
    mode_ = Mode::Normal;
    command_.clear();
}

bool Editor::SaveBuffer(const std::filesystem::path& path) {
    try{
        if (path.empty()) {
            buffer_.Save();
        } else {
            buffer_.SaveAs(path);
        }
    } catch (const std::exception& error) {
        message_ = error.what();
        return false;
    }

    message_ = "\"" + buffer_.GetDisplayName() + "\" " + std::to_string(buffer_.GetLineCount()) + " lines written";
    return true;
}

} // namespace sjtu
