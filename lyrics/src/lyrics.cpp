#define  _CRT_SECURE_NO_WARNINGS

#include "lyrics.h"
#include <sstream>
#include <algorithm>
#include "../external/json/document.h"
#include "../external/json/stringbuffer.h"
#include "../external/json/writer.h"
#include "../external/zlib/include/zlib.h"

#pragma comment(lib, "src/external/zlib/prebuilt/libzlib.lib")

static const uint8_t gs_byKey[] = {
    64, 71, 97, 119, 94, 50, 116, 71, 81, 54, 49, 45, 206, 210, 110, 105
};

static uint8_t *decrypt_bytes(uint8_t *data, size_t length) {
    unsigned i;
    for (i = 0U; i < length; ++i) {
        data[i] ^= gs_byKey[i & 15];
    }
    return data;
}

static bool decode_buffer(std::vector<uint8_t> data, std::string &str) {
    if (data.size() < 4) {
        return false;
    }

    if (memcmp(data.data(), "krc1", 4) != 0) {
        return false;
    }

    memmove(&data[0], &data[4], data.size() - 4);  // 去掉头4字节
    memset(&data[data.size() - 4], 0, 4);  // 最后补4个0
    decrypt_bytes(data.data(), data.size());  // 解密

    str.resize(data.size() * 8);

    uLongf destLen = static_cast<uLongf>(str.size());
    int ret = uncompress(reinterpret_cast<Bytef *>(&str[0]), &destLen, reinterpret_cast<const Bytef *>(data.data()), static_cast<uLong>(data.size() - 4));  // 解压
    if (ret != Z_OK) {
        return false;
    }

    if (destLen > 3 && strncmp(str.c_str(), "\xEF\xBB\xBF", 3) == 0) {  // BOM
        str.erase(0, 3);
        destLen -= 3;
    }

    str.resize(destLen);
    return true;
}

static bool encode_buffer(const std::string &str, std::vector<uint8_t> &output) {
    std::vector<uint8_t> temp;
    uLongf destLen = static_cast<uLongf>(str.length());
    temp.resize(destLen);
    int ret = compress(reinterpret_cast<Bytef *>(temp.data()), &destLen, reinterpret_cast<const Bytef *>(str.c_str()), destLen);
    if (ret != Z_OK) {
        return false;
    }

    temp.resize(destLen);
    decrypt_bytes(temp.data(), destLen);

    temp.resize(temp.size() + 4);
    memmove(&temp[4], &temp[0], destLen);
    memcpy(&temp[0], "krc1", 4);

    temp.shrink_to_fit();
    output.swap(temp);
    return true;
}

static bool parse_tags(const std::string &line,  std::unordered_map<std::string, std::string> *tags) {
    if (line.empty() || line.length() < 3 || line.front() != L'[' || line.back() != L']') {
        return false;
    }
    std::string::size_type pos = line.find(L':', 1);
    if (pos == std::string::npos) {
        return false;
    }

    if (pos > 1 && pos + 2 < line.length()) {
        tags->insert(std::make_pair(line.substr(1, pos - 1), line.substr(pos + 1, line.length() - pos - 2)));
    }
    return true;
}

static std::string stringify_tags(std::unordered_map<std::string, std::string> tags, unsigned total, int offset, const std::string &language) {
    std::stringstream ss;

    static const struct {
        const char *key;
        const char *value;
    } metaOrder[] = {
        {"id", "$00000000"},
        {"ar",""},  // 艺术家
        {"ti",""},  // 标题
        {"by",""},  // 制作人
        // {"hash","00000000000000000000000000000000"}  // 应该是md5，但酷狗播放器未做验证，甚至没有都可以
    };

    for (size_t i = 0, cnt = sizeof(metaOrder) / sizeof(*metaOrder); i < cnt; ++i) {
        auto it = tags.find(metaOrder[i].key);
        if (it != tags.end()) {
            ss << '[' << it->first << ':' << it->second << ']' << std::endl;
            tags.erase(it);
        } else {
            ss << '[' << metaOrder[i].key << ':' << metaOrder[i].value << ']' << std::endl;
        }
    }

    ss << "[total:" << total << ']' << std::endl;
    if (offset != 0) {
        ss << "[offset:" << offset << ']' << std::endl;
    }
    if (!language.empty()) {
        ss << "[language:" << language << ']' << std::endl;
    }

    // TODO: 其他数据
    //for (auto it = tags.begin(); it != tags.end() ++it) {
    //
    //}

    return ss.str();
}

static bool parse_sentence(const char *line, lyrics_sentence_t *sentence) {
    int tagLen = 0;
    int ret = sscanf(line, "[%d,%d]%n", &(sentence->start_time), &(sentence->duration), &tagLen);  // 读取当前一句的开始时间和持续时间
    if (ret != 2 || tagLen == 0) {  // 读取错误
        return false;
    }

    std::vector<lyrics_word_t> &words = sentence->words;
    lyrics_word_t temp;

    for (const char *p = strchr(line + tagLen, '<'); p != nullptr; ) {
        tagLen = 0;
        ret = sscanf(p, "<%d,%d,%d>%n", &temp.start_time, &temp.duration, &temp.unknown, &tagLen);  // 读取字的开始时间和持续时间
        if (ret != 3 || tagLen == 0) {  // 读取错误
            return false;
        }

        p += tagLen;
        if (*p == '\r' || *p == '\n' || *p == '\0') {  // 没有字
            return false;
        }

        while (*p == ' ') continue;  // 去掉前导空格

        // 到下一个<之前的全是字
        const char *q = strchr(p, '<');
        if (q == nullptr) {
            temp.text.assign(p);
        }
        else {
            temp.text.assign(p, q);
        }
        words.push_back(std::move(temp));
        p = q;
    }

    if (!words.empty()) {
        // trim最后一个字
        std::string &text = words.back().text;
        while (text.back() == ' ') {
            text.pop_back();
        }
    }

    return true;
}

static std::string &stringify_sentences(std::string &str, const std::vector<lyrics_sentence_t> &sentences) {
    for (auto &sentence : sentences) {
        str.push_back('[');
        str.append(std::to_string(sentence.start_time));
        str.push_back(',');
        str.append(std::to_string(sentence.duration));
        str.push_back(']');
        for (auto &word : sentence.words) {
            str.push_back('<');
            str.append(std::to_string(word.start_time));
            str.push_back(',');
            str.append(std::to_string(word.duration));
            str.push_back(',');
            str.append(std::to_string(word.unknown));
            str.push_back('>');
            str.append(word.text);
        }
        str.append("\n");
    }

    return str;
}

// ================= base64 begin =================

static const uint8_t alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::vector<uint8_t> base64_decode(const char *input, size_t len, int *err) {
    static char inalphabet[256], decoder[256];

    for (int i = static_cast<int>(sizeof alphabet) - 1; i >= 0 ; --i) {
        inalphabet[alphabet[i]] = 1;
        decoder[alphabet[i]] = i;
    }

    std::vector<uint8_t> output;
    output.reserve(len / 4 * 3 + 1);

    int errors = 0;
    int char_count = 0;
    int bits = 0;
    int ch = 0;
    for (size_t i = 0; i < len; ++i) {
        ch = input[i];
        if (ch == '=') {
            break;
        }
        if (ch > 255 || ! inalphabet[ch]) {
            continue;
        }
        bits += decoder[ch];
        ++char_count;
        if (char_count == 4) {
            output.push_back(bits >> 16);
            output.push_back((bits >> 8) & 0xFF);
            output.push_back(bits & 0xFF);
            bits = 0;
            char_count = 0;
        } else {
            bits <<= 6;
        }
    }

    if (ch == '=') {
        switch (char_count) {
        case 1:
            fprintf(stderr, "base64_decode: encoding incomplete: at least 2 bits missing");
            ++errors;
            break;
        case 2:
            output.push_back(bits >> 10);
            break;
        case 3:
            output.push_back(bits >> 16);
            output.push_back((bits >> 8) & 0xFF);
            break;
        }
    }

    if (err) {
        *err = errors;
    }
    return output;
}

static std::string base64_encode(const uint8_t *input, size_t len) {
    std::string output;
    output.reserve((len + 2) / 3 * 4);

    size_t char_count = 0;
    uint32_t bits = 0;
    for (size_t i = 0; i < len; ++i) {
        bits |= input[i];

        ++char_count;
        if (char_count == 3) {
            output.push_back(alphabet[(bits >> 18) & 0x3F]);
            output.push_back(alphabet[(bits >> 12) & 0x3F]);
            output.push_back(alphabet[(bits >> 6) & 0x3F]);
            output.push_back(alphabet[bits & 0x3F]);
            bits = 0;
            char_count = 0;
        } else {
            bits <<= 8;
        }
    }

    if (char_count != 0) {
        if (char_count == 1) {
            bits <<= 8;
        }

        output.push_back(alphabet[(bits >> 18) & 0x3F]);
        output.push_back(alphabet[(bits >> 12) & 0x3F]);
        if (char_count > 1) {
            output.push_back(alphabet[(bits >> 6) & 0x3F]);
        } else {
            output.push_back('=');
        }
        output.push_back('=');
    }

    return output;
}

// ================= base64 end =================

static void parse_phonetics(const rapidjson::Value &phonetic, std::vector<lyrics_sentence_t> &sentences) {
    for (rapidjson::SizeType i = 0, cnt = std::min(phonetic.Size(), static_cast<rapidjson::SizeType>(sentences.size())); i < cnt; ++i) {
        const rapidjson::Value &p = phonetic[i];
        std::vector<lyrics_word_t> &words = sentences[i].words;
        if (p.IsArray()) {
            for (rapidjson::SizeType k = 0, n = std::min(p.Size(), static_cast<rapidjson::SizeType>(words.size())); k < n; ++k) {
                words[k].phonetic = p[k].GetString();
            }
        }
    }
}

static void jsonify_phonetics(const std::vector<lyrics_sentence_t> &sentences, rapidjson::Value &phonetic, rapidjson::Value::AllocatorType &alloc) {
    for (auto &sentence : sentences) {
        rapidjson::Value words(rapidjson::Type::kArrayType);
        for (auto &word : sentence.words) {
            words.PushBack(rapidjson::StringRef(word.phonetic.c_str()), alloc);
        }
        phonetic.PushBack(std::move(words), alloc);
    }
}

static bool parse_language(const std::string &language, std::vector<lyrics_sentence_t> &sentences) {
    std::vector<uint8_t> buf = base64_decode(language.c_str(), language.length(), nullptr);
    buf.push_back('\0');

    try {
        rapidjson::Document doc;
        doc.Parse<0>((const char *)buf.data());
        if (doc.HasParseError()) {
            return false;
        }

        if (!doc.IsObject()) {
            return false;
        }

        rapidjson::Value::ConstMemberIterator it = doc.FindMember("content");
        if (it == doc.MemberEnd() || !it->value.IsArray()) {
            return false;
        }

        rapidjson::Value::ConstArray content = it->value.GetArray();
        if (content.Empty()) {
            return false;
        }

        for (rapidjson::SizeType i = 0, cnt = content.Size(); i < cnt; ++i) {
            const rapidjson::Value &text = content[i];
            if (!text.IsObject()) {
                continue;
            }

            it = text.FindMember("type");
            if (it != text.MemberEnd() && it->value.IsUint()) {
                switch (it->value.GetUint()) {
                case 0:  // 0为音译
                    it = text.FindMember("lyricContent");
                    if (it != text.MemberEnd() && it->value.IsArray()) {
                        parse_phonetics(it->value, sentences);
                    }
                    break;
                case 1:  // 1为翻译
                    //it = text.FindMember("lyricContent");
                    //if (it != text.MemberEnd() && it->value.IsArray()) {
                    //    parse_translation(it->value, sentences);
                    //}
                    break;
                default:
                    break;
                }
            }
        }

        return true;
    }
    catch (std::exception &e) {
        fprintf(stderr, "%s %s", __FUNCTION__, e.what());
        return false;
    }
}

static std::string stringify_language(const std::vector<lyrics_sentence_t> &sentences) {
    bool hasPhonetic = std::any_of(sentences.begin(), sentences.end(), [](const lyrics_sentence_t &sentence) { return std::any_of(sentence.words.begin(), sentence.words.end(), [](const lyrics_word_t &word) { return !word.phonetic.empty(); }); });
    bool hasTranslation = std::any_of(sentences.begin(), sentences.end(), [](const lyrics_sentence_t &sentence) { return !sentence.translation.empty(); });
    if (!hasPhonetic && !hasTranslation) {
        return "";
    }

    rapidjson::Document doc;
    rapidjson::Value content(rapidjson::Type::kArrayType);
    if (hasPhonetic) {
        rapidjson::Value obj(rapidjson::Type::kObjectType);

        rapidjson::Value lyricContent(rapidjson::Type::kArrayType);
        jsonify_phonetics(sentences, lyricContent, doc.GetAllocator());
        obj.AddMember("lyricContent", std::move(lyricContent), doc.GetAllocator());
        obj.AddMember("type", rapidjson::Value(1), doc.GetAllocator());
        obj.AddMember("version", rapidjson::Value(1), doc.GetAllocator());

        content.PushBack(std::move(obj), doc.GetAllocator());
    }

    if (hasTranslation) {
        // TODO:
    }

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);

    return base64_encode((const uint8_t *)buf.GetString(), buf.GetSize());
}

bool lyrics_decode(const std::vector<uint8_t> &data, lyrics_detail_t *lyrics) {
    std::string str;
    if (!decode_buffer(data, str)) {
        return false;
    }

    std::stringstream ss(std::move(str));

    std::string line;
    while (std::getline(ss, line)) {
        if (line.back() == L'\r') {
            line.pop_back();
        }

        if (parse_tags(line, &lyrics->tags)) {
            continue;
        }

        lyrics_sentence_t sentence;
        if (parse_sentence(line.c_str(), &sentence)) {
            lyrics->sentences.push_back(std::move(sentence));
        }
    }

    auto it = lyrics->tags.find("offset");
    if (it != lyrics->tags.end()) {
        lyrics->offset = atoi(it->second.c_str());
        lyrics->tags.erase(it);
    }

    it = lyrics->tags.find("total");
    if (it != lyrics->tags.end()) {
        lyrics->total_time = atoi(it->second.c_str());
        lyrics->tags.erase(it);
    }

    it = lyrics->tags.find("language");
    if (it != lyrics->tags.end()) {
        parse_language(it->second, lyrics->sentences);
        lyrics->tags.erase(it);
    }

    return true;
}

bool lyrics_encode(const lyrics_detail_t *lyrics, std::vector<uint8_t> &data) {
    std::string str;

    std::string language = stringify_language(lyrics->sentences);
    str.append(stringify_tags(lyrics->tags, lyrics->total_time, lyrics->offset, language));
    stringify_sentences(str, lyrics->sentences);

    return encode_buffer(str, data);
}
