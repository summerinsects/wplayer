#ifdef _MSC_VER
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <iostream>
#include "../lyrics/lyrics.h"
#include "../lyrics/lyrics.cpp"

#ifdef _MSC_VER
#ifdef _DEBUG
//#pragma comment(lib, "../Debug/lyrics.lib")
#else
//#pragma comment(lib, "../Release/lyrics.lib")
#endif
#endif

static void convert();

int main(void)
{
    system("chcp 65001");

    convert();
    return 0;

    FILE *fp = fopen("test.krc", "rb");
    if (fp == nullptr) {
        printf("打开文件失败\n");
        return 0;
    }
    fseek(fp, SEEK_SET, SEEK_END);
    long size = ftell(fp);
    std::vector<uint8_t> fileData(size);

    fseek(fp, SEEK_SET, SEEK_SET);  // 文件指针定位到文件头
    fread(fileData.data(), 1, size, fp);

    fclose(fp);

    lyrics_info_t info;
    lyrics_decode(fileData, &info);

    //for (auto &m : info.metadata) {
    //    printf("%s %s\n", m.first.c_str(), m.second.c_str());
    //}

    //for (auto &s : info.sentences) {
    //    for (auto &w : s.words) {
    //        printf("%s %s|", w.text.c_str(), w.phonetic.c_str());
    //    }
    //    puts("");
    //}
    //puts("");


    system("pause");
    return 0;
}

static void convert() {
    FILE *fp = fopen("cleartext.txt", "rb");
    if (fp == nullptr) {
        printf("打开文件失败\n");
        return;
    }
    fseek(fp, SEEK_SET, SEEK_END);
    long size = ftell(fp);
    std::vector<uint8_t> fileData(size);

    fseek(fp, SEEK_SET, SEEK_SET);  // 文件指针定位到文件头
    fread(fileData.data(), 1, size, fp);

    fclose(fp);

    std::vector<uint8_t> temp;
    encode_buffer(std::string(fileData.begin(), fileData.end()), temp);

    fp = fopen("ciphertext.krc", "wb");
    if (fp == nullptr) {
        printf("打开文件失败\n");
        return;
    }
    fwrite(temp.data(), temp.size(), 1, fp);
    fclose(fp);
}
