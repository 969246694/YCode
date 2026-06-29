#ifndef YCODE_INPUT_H
#define YCODE_INPUT_H

namespace ycode {

enum class Key : int {
    Backspace = 0x08,
    Tab = 0x09,
    Enter = 0x0D,
    Escape = 0x1B,
    Space = 0x20,
    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28,
    A = 'A',
    D = 'D',
    S = 'S',
    W = 'W'
};

} // namespace ycode

#endif // YCODE_INPUT_H
