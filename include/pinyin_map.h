#ifndef PINYIN_MAP_H
#define PINYIN_MAP_H

#include <string>

namespace ChinesePinyin {

    // 返回汉字的第一个拼音(小写); 未收录返回空字符串
    // 数据在 pinyin_data.cpp (constexpr 有序数组 + 二分查找),
    // 避免 header 内嵌 6500+ 条 std::map 动态初始化导致的编译/启动开销
    std::string getFirstPinyin(char32_t codepoint);
}

#endif // PINYIN_MAP_H