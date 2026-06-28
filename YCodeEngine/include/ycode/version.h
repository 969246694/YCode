#ifndef YCODE_VERSION_H
#define YCODE_VERSION_H

namespace ycode {

struct Version {
    int major;
    int minor;
    int patch;
};

inline Version getVersion()
{
    return {0, 1, 0};
}

} // namespace ycode

#endif // YCODE_VERSION_H

