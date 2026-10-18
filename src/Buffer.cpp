#include <fstream>

#include "Buffer.hpp"

namespace sjtu {

Buffer::Buffer(const std::filesystem::path& path){
    if (path.empty()) {
        lines_.push_back("");
    } else {
        std::ifstream file(path);
        if (!file.is_open()) {
            lines_.push_back("");
        } else {
            std::string line;
            while (std::getline(file, line)) {
                lines_.push_back(line);
            }
            if (lines_.empty()) {
                lines_.push_back("");
            }
        }
    }
}

Buffer::Buffer(std::vector<std::string> lines, std::filesystem::path path) {
    //从lines构造Buffer,并设置path为文件路径
    lines_ = std::move(lines);
    path_ = std::move(path);
}
/*AI:可以写：
Buffer::Buffer(std::vector<std::string> lines, std::filesystem::path path) 
    ：lines_(std::move(lines)), path_(std::move(path)) {
    //构造函数体可以为空，因为成员初始化列表已经完成了初始化
}
*/

std::size_t Buffer::GetLineCount() const {
    //返回文件行数
    return lines_.size();
}

const std::string& Buffer::GetLineAt(std::size_t row) const {
    //返回第row行的内容
    return lines_.at(row); //使用at()可以自动进行越界检查!!!
}


std::string Buffer::GetDisplayName() const {
    //返回文件名,若是新文件,返回"[No Name]"
    if (path_.empty()) {
        return "[No Name]";
    }
    return path_.filename().string();//.string()用于将std::filesystem::path转换为std::string
}

bool Buffer::IsModified() const {
    //返回文件和上次保存比起来是否被修改过
    return is_modified_;
}

void Buffer::InsertCharacter(std::size_t row, std::size_t column, char value) {
    //在第row行第col列插入一个value, 注意越界检查
    if (row >= lines_.size())    return; //越界检查
    if (column > lines_[row].size()) return; //越界检查可以在行尾插入字符

    lines_[row].insert(lines_[row].begin() + column, value);
    is_modified_ = true; //标记文件已被修改!!!
}

void Buffer::EraseCharacter(std::size_t row, std::size_t column) {
   //在第row行第col列删除一个value
    if (row >= lines_.size())    return; //越界检查
    if (column >= lines_[row].size()) return; //越界检查

    lines_[row].erase(lines_[row].begin() + column);
    is_modified_ = true; //标记文件已被修改!!!
}

void Buffer::SplitLine(std::size_t row, std::size_t column) {
    //在第row行第col列分割,即在此处敲了回车键
    if (row >= lines_.size())    return; //越界检查
    if (column > lines_[row].size()) return; 

    std::string current_line = lines_[row];
    std::string next_line = current_line.substr(column); //获取分割后的新行;substr(起始位置，长度)
    lines_[row] = current_line.substr(0, column); //保留分割前的部分
    lines_.insert(lines_.begin() + row + 1, next_line); //在当前行下方插入新行
    is_modified_ = true; //标记文件已被修改!!!
}

void Buffer::JoinLine(std::size_t row) {
    //把第row + 1行合并进第row行
    if (row + 1 >= lines_.size()) return; //越界检查
    
    lines_[row] += lines_[row + 1]; //将下一行内容追加到当前行
    lines_.erase(lines_.begin() + row + 1); //删除下一行
    is_modified_ = true; //标记文件已被修改!!!
}

void Buffer::Save() {
    //把文件内容保存, 直接调用WriteTo方法
    WriteTo(path_);
    is_modified_ = false; //标记文件已保存
}

void Buffer::SaveAs(const std::filesystem::path& path) {
    WriteTo(path);
    path_ = path; //更新当前Buffer的路径
    is_modified_ = false; //标记文件已保存
}


void Buffer::WriteTo(const std::filesystem::path& path) const {
   std::ofstream ofs(path);
   if (!ofs.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }
    
    //只补齐行之间的换行，最后一行绝不乱加换行，完全保持 lines_ 的原始状态
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        ofs << lines_[i];
        if (i + 1 < lines_.size()) {
            ofs << '\n';
        }
    }
}
} // namespace sjtu
