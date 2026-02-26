//
// Created by cpasjuste on 05/04/19.
//

#include <cstdarg>
#include <zlib.h>
#include "ss_api.h"
#include "pinyin_map.h"
#include <algorithm> 


using namespace ss_api;

std::string Api::ss_devid;
std::string Api::ss_devpassword;
std::string Api::ss_softname;
bool ss_debug = false;

std::string
Api::getXmlAttrStr(tinyxml2::XMLElement *element, const std::string &name, const std::string &defaultValue) {
    if (element == nullptr || element->Attribute(name.c_str()) == nullptr) {
        return defaultValue;
    }

    return element->Attribute(name.c_str());
}

int Api::getXmlAttrInt(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseInt(attr);
}

long Api::getXmlAttrLong(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseLong(attr);
}

unsigned long Api::getXmlAttrULong(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseULong(attr);
}

bool Api::getXmlAttrBool(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseBool(attr);
}

std::string Api::getXmlTextStr(tinyxml2::XMLElement *element, const std::string &defaultValue) {
    if (element == nullptr || element->GetText() == nullptr) {
        return defaultValue;
    }

    return element->GetText();
}

int Api::getXmlTextInt(tinyxml2::XMLElement *element) {
    std::string text = getXmlTextStr(element);
    return parseInt(text);
}

float Api::getXmlTextFloat(tinyxml2::XMLElement *element) {
    std::string text = getXmlTextStr(element);
    return parseFloat(text);
}

bool Api::getXmlTextBool(tinyxml2::XMLElement *element) {
    std::string text = getXmlTextStr(element);
    return parseBool(text);
}

tinyxml2::XMLElement *Api::addXmlElement(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *parent,
                                         const std::string &name, const std::string &value) {
    if (parent == nullptr) {
        return nullptr;
    }

    if (!value.empty()) {
        tinyxml2::XMLElement *element = doc->NewElement(name.c_str());
        element->SetText(value.c_str());
        parent->InsertEndChild(element);
        return element;
    }

    return nullptr;
}

bool Api::sortByName(const std::string &g1, const std::string &g2) {
#ifdef _MSC_VER
    return _stricmp(g1.c_str(), g2.c_str()) < 0;
#else
    return strcasecmp(g1.c_str(), g2.c_str()) < 0;
#endif
}

bool Api::sortInteger(int i1, int i2) {
    return i1 < i2;
}

bool Api::sortGameByName(const Game &g1, const Game &g2) {
    const std::string lhs = g1.name;
    const std::string rhs = g2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortGameByPath(const Game &g1, const Game &g2) {
    const std::string lhs = g1.path;
    const std::string rhs = g2.path;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortSystemByName(const System &s1, const System &s2) {
    const std::string lhs = s1.name;
    const std::string rhs = s2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortEditorByName(const Game::Editor &e1, const Game::Editor &e2) {
    const std::string lhs = e1.name;
    const std::string rhs = e2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortDeveloperByName(const Game::Developer &d1, const Game::Developer &d2) {
    const std::string lhs = d1.name;
    const std::string rhs = d2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortGenreByName(const Game::Genre &g1, const Game::Genre &g2) {
    const std::string lhs = g1.name;
    const std::string rhs = g2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

int Api::parseInt(const std::string &str, int defValue) {
    char *end = nullptr;
    long i = strtol(str.c_str(), &end, 10);
    if (end != nullptr && end != str.c_str()) {
        return (int) i;
    }
    return defValue;
}

long Api::parseLong(const std::string &str, long defValue) {
    char *end = nullptr;
    long i = strtol(str.c_str(), &end, 10);
    if (end != nullptr && end != str.c_str()) {
        return i;
    }
    return defValue;
}

unsigned long Api::parseULong(const std::string &str, unsigned long defValue) {
    char *end = nullptr;
    unsigned long i = strtoll(str.c_str(), &end, 10);
    if (end != nullptr && end != str.c_str()) {
        return i;
    }
    return defValue;
}

float Api::parseFloat(const std::string &str, float defValue) {
    char *end = nullptr;
    float f = strtof(str.c_str(), &end);
    if (end != nullptr && end != str.c_str()) {
        return f;
    }
    return defValue;
}

bool Api::parseBool(const std::string &str, bool defValue) {
    if (str.empty() || str != "true" || str != "false" || str != "0" || str != "1") {
        return defValue;
    }

    return str == "true" || str == "1";
}

std::string Api::getFileCrc(const std::string &zipPath) {
    unsigned char buffer[BUFSIZ];
    char hex[16];
    size_t size;
    FILE *pFile;

    memset(hex, 0, 16);

#ifdef _MSC_VER
    fopen_s(&pFile, zipPath.c_str(), "rb");
#else
    pFile = fopen(zipPath.c_str(), "rb");
#endif
    if (!pFile) {
        return hex;
    }

    uLong crc = crc32(0L, Z_NULL, 0);
    while ((size = fread(buffer, 1, BUFSIZ, pFile)) != 0) {
        crc = crc32(crc, buffer, size);
    }
    snprintf(hex, 16, "%08lx", crc);

    fclose(pFile);

    return hex;
}

#ifdef __WINDOWS__

void Api::printc(int color, const char *format, ...) {
#else

void Api::printc(const char *color, const char *format, ...) {
#endif

    char buffer[1024];
    va_list arg;
    va_start(arg, format);
    vsnprintf(buffer, 1024, format, arg);
    va_end(arg);

#ifdef __WINDOWS__
    WORD consoleAttr = FOREGROUND_INTENSITY | FOREGROUND_RED
                       | FOREGROUND_GREEN | FOREGROUND_BLUE;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        consoleAttr = csbi.wAttributes;
    }
    SetConsoleTextAttribute(h, color);
    printf("%s", buffer);
    SetConsoleTextAttribute(h, consoleAttr);
#else
    printf("%s%s" "\033[0m", color, buffer);
#endif
}

void Api::printe(int code, int delay) {
    if (code == 429) {
        printc(COLOR_O, "NOK: maximum requests per minute reached... retrying in %i seconds\n", delay);
    } else if (code == 28) {
        printc(COLOR_O, "NOK: timeout reached... retrying in %i seconds\n", delay);
    }
}

bool Api::sortGameByNamePinYin(const Game &g1, const Game &g2) {
    const std::string &lhs = g1.name;
    const std::string &rhs = g2.name;
    
    auto lhs_it = lhs.begin();
    auto rhs_it = rhs.begin();
    
    while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
        // 检查当前字符是否为ASCII（英文）
        bool lhs_is_ascii = (*lhs_it & 0x80) == 0;
        bool rhs_is_ascii = (*rhs_it & 0x80) == 0;
        
        // 情况1：两个都是ASCII字符（英文）
        if (lhs_is_ascii && rhs_is_ascii) {
            char lhs_lower = std::tolower(static_cast<unsigned char>(*lhs_it));
            char rhs_lower = std::tolower(static_cast<unsigned char>(*rhs_it));
            
            if (lhs_lower != rhs_lower) {
                return lhs_lower < rhs_lower;
            }
            
            ++lhs_it;
            ++rhs_it;
            continue;
        }
        
        // 情况2：一个是ASCII，一个是非ASCII
        if (lhs_is_ascii || rhs_is_ascii) {
            // ASCII字符（英文）排序在非ASCII（中文）之前
            return lhs_is_ascii;
        }
        
        // 情况3：两个都是非ASCII字符（中文）
        // UTF-8解码函数
        auto decode_utf8 = [](std::string::const_iterator& it, const std::string::const_iterator end) -> char32_t {
            if (it == end) return 0;
            
            unsigned char c = *it++;
            if (c < 0x80) return c;
            
            char32_t result = 0;
            int remaining = 0;
            
            if ((c & 0xE0) == 0xC0) {
                result = c & 0x1F;
                remaining = 1;
            } else if ((c & 0xF0) == 0xE0) {
                result = c & 0x0F;
                remaining = 2;
            } else if ((c & 0xF8) == 0xF0) {
                result = c & 0x07;
                remaining = 3;
            } else {
                return 0xFFFD; // 替换字符
            }
            
            for (int i = 0; i < remaining; ++i) {
                if (it == end || (*it & 0xC0) != 0x80) return 0xFFFD;
                result = (result << 6) | (*it++ & 0x3F);
            }
            
            return result;
        };
        
        char32_t lhs_char = decode_utf8(lhs_it, lhs.end());
        char32_t rhs_char = decode_utf8(rhs_it, rhs.end());
        
        // 获取汉字拼音（如果有）
        std::string lhs_py;
        std::string rhs_py;
        
        auto lhs_py_it = ChinesePinyin::hanziToPinyin.find(lhs_char);
        if (lhs_py_it != ChinesePinyin::hanziToPinyin.end() && !lhs_py_it->second.empty()) {
            lhs_py = lhs_py_it->second[0]; // 使用第一个拼音
        }
        
        auto rhs_py_it = ChinesePinyin::hanziToPinyin.find(rhs_char);
        if (rhs_py_it != ChinesePinyin::hanziToPinyin.end() && !rhs_py_it->second.empty()) {
            rhs_py = rhs_py_it->second[0]; // 使用第一个拼音
        }
        
        // 转换为小写比较
        std::transform(lhs_py.begin(), lhs_py.end(), lhs_py.begin(), ::tolower);
        std::transform(rhs_py.begin(), rhs_py.end(), rhs_py.begin(), ::tolower);
        
        // 比较逻辑
        if (!lhs_py.empty() && !rhs_py.empty()) {
            // 两个都有拼音，比较拼音
            if (lhs_py != rhs_py) {
                return lhs_py < rhs_py;
            }
        } else if (lhs_py.empty() != rhs_py.empty()) {
            // 一个有拼音一个没有，有拼音的排前面
            return !lhs_py.empty();
        }
        
        // 拼音相同或都无法获取拼音，比较原始字符
        if (lhs_char != rhs_char) {
            return lhs_char < rhs_char;
        }
    }
    
    // 一个字符串是另一个的前缀，较短的排在前面
    return lhs.size() < rhs.size();
}








