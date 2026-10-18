/*
Terminal.hpp
Terminal封装与终端有关的系统调用,把输入字节解析成KeyEvent,并负责输出画面.
构造时保存终端设置并开启raw模式,析构时恢复设置,这样正常退出或异常展开时都能清理.
*/
/*raw模式：
    1. 关闭回显,输入的字符不会显示在终端上
    2. 关闭行缓冲,输入的字符会立即传给程序
    3. 关闭信号,按Ctrl-C不会终止程序,按Ctrl-Z不会挂起程序
    4. 关闭特殊字符处理,按退格不会删除前一个字符等等
*/
#ifndef MINIVIM_TERMINAL_HPP
#define MINIVIM_TERMINAL_HPP

#include "Key.hpp"
#include "Types.hpp"

#include <optional>
#include <string_view>
#include <termios.h>

namespace sjtu {

class Terminal {
public:
    Terminal();
    ~Terminal() noexcept;//构造/析构函数

    //一个对象负责恢复终端设置,禁止复制和移动,避免多个对象重复管理同一份状态
    Terminal(const Terminal&) = delete;//禁止拷贝构造
    Terminal& operator=(const Terminal&) = delete;//禁止拷贝赋值
    Terminal(Terminal&&) = delete;//禁止移动构造
    Terminal& operator=(Terminal&&) = delete;//禁止移动赋值（move）

    //等待并解析一个按键,特殊键可能由多个输入字节组成
    KeyEvent ReadKey();
    ScreenSize GetScreenSize();
    void WriteOutput(std::string_view output);
    void ClearScreen();

private:
    char ReadByte(); //持续尝试,直到读到一个字节或遇到错误
    std::optional<char> TryReadByte(); //等待一次读取超时后可返回nullopt,不是立即返回的非阻塞读取//试探性读取器
    ScreenSize QueryCursorPosition(); //查询终端光标坐标,供获取屏幕大小的备用路径使用//排版基础

    termios original_{}; //进入raw模式前的终端设置,用于恢复
};

} // namespace sjtu

#endif // MINIVIM_TERMINAL_HPP
