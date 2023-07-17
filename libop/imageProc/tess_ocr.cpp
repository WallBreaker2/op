#include "tess_ocr.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include "../core/helpfunc.h"
tess_ocr::tess_ocr(): m_api(nullptr)   {
    init();
}
tess_ocr::~tess_ocr() {
    release();
}
int tess_ocr::init() {
    // 创建Tesseract OCR对象
    m_api = new tesseract::TessBaseAPI();
    // 初始化Tesseract OCR
    if (m_api->Init("./tess_model", "chi_sim")) {
        setlog("Could not initialize tesseract.\n");
        release();
    }
    return 0;
}
int tess_ocr::release() {
    if (m_api) {
        // 删除Tesseract OCR对象
        delete m_api;
    }
    m_api = nullptr;
    return 0;
}

static string utf8_to_ansi(string strUTF8);

int tess_ocr::ocr(byte* data, int w, int h, int bpp, vector<tess_rec_info>& result) {
    result.clear();
    if (m_api == nullptr)return -1;
    
    // 将图像数据设置给Tesseract OCR
    m_api->SetImage(data, w, h, bpp, w * bpp);

    // 执行文字识别
    m_api->Recognize(0);

    // 获取结果迭代器
    tesseract::ResultIterator* ri = m_api->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
    //std::setlocale()
    // 如果结果迭代器有效，则循环
    if (ri != 0) {
        do {
            // 获取识别出的单词和置信度
            const char* word = ri->GetUTF8Text(level);
            int x1, x2, y1, y2;
            ri->BoundingBox(level, &x1, &y1, &x2, &y2);
            float conf = ri->Confidence(level);

            // 显示单词和置信度
            //printf("word: '%s';  \tconf: %.2f box=[%d,%d %d,%d]; \n", word, conf, x1, y1, x2, y2);
            //std::wcout << L"xxx:" << utf82ws(word) << std::endl;
            tess_rec_info ts;
            ts.confidenc = conf;
            ts.left_top = point_t(x1, y1);
            ts.right_bottom = point_t(x2, y2);
            ts.text = _s2wstring(utf8_to_ansi(word));
            result.push_back(ts);
            // 释放单词的内存
            delete[] word;
        } while (ri->Next(level));
    }
    //Pix* px = m_api->GetInputImage();
    //pixWrite("test_oux.bmp", px, IFF_BMP);
    return result.size();
}

string utf8_to_ansi(string strUTF8) {
    UINT nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, NULL, NULL);
    WCHAR* wszBuffer = new WCHAR[nLen + 1];
    nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, wszBuffer, nLen);
    wszBuffer[nLen] = 0;
    nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
    CHAR* szBuffer = new CHAR[nLen + 1];
    nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
    szBuffer[nLen] = 0;
    strUTF8 = szBuffer;
    delete[]szBuffer;
    delete[]wszBuffer;
    return strUTF8;
}