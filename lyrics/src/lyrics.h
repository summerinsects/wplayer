#ifndef __LYRICS_H__
#define __LYRICS_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>

struct lyrics_word_t {
    std::string text;  // 字
    std::string phonetic;  // 音译
    int start_time = 0;  // 开始时间
    int duration = 0;  // 持续时间
    int unknown = 0;  // 这个值永远为0，未知其含义
};

struct lyrics_sentence_t {
    int start_time = 0;  // 开始时间
    int duration = 0;  // 持续时间
    std::vector<lyrics_word_t> words;  // 内容
    std::string translation;  // 翻译
};

struct lyrics_detail_t {
    std::unordered_map<std::string, std::string> tags;
    unsigned total_time = 0;
    int offset = 0;
    std::vector<lyrics_sentence_t> sentences;
};

bool lyrics_decode(const std::vector<uint8_t> &data, lyrics_detail_t *lyrics);
bool lyrics_encode(const lyrics_detail_t *lyrics, std::vector<uint8_t> &data);

#endif
