#include "OcrWrapper.h"
#include "../core/helpfunc.h"
#include "../core/opEnv.h"
#include <iostream>

ocr_engine_init_t OcrWrapper::ocr_engine_init;
ocr_engine_ocr_t OcrWrapper::ocr_engine_ocr;
ocr_engine_release_t OcrWrapper::ocr_engine_release;

OcrWrapper::OcrWrapper() : m_engine(nullptr) {
	//paddle
#ifdef _M_X64
	std::wstring paddle_path = opEnv::getBasePath() + L"/paddle";
	auto dllName =  L"paddle_ocr.dll";
	std::wstring root = paddle_path;
	wstring detName = root + L"/models/ch_PP-OCRv3_det_infer";
	wstring recName = root + L"/models/ch_PP-OCRv3_rec_infer";
	wstring otherName = root + L"/utils/ppocr_keys_v1.txt";
	vector<string> argvs = {
		"tests",
		"--det_model_dir=" + _ws2string(detName),
		"--rec_model_dir=" + _ws2string(recName),
		"--rec_char_dict_path=" + _ws2string(otherName),
		"--enable_mkldnn=true"
	};
#else
	//tess
	std::wstring paddle_path = opEnv::getBasePath() + L"/tess";
	auto dllName = L"tess_engine.dll";
	std::wstring root = opEnv::getBasePath()+L"/tess";
	vector<string> argvs = {
		"tests",
		_ws2string(root)+"/tess_model",
		"chi_sim"
	};
#endif
	init(paddle_path, dllName, argvs);
}
OcrWrapper::~OcrWrapper() {
	release();
}
int OcrWrapper::init(const std::wstring& engine, const std::wstring& dllName, const vector<string>& argvs) {
	using std::cout;
	using std::endl;
	//只需加载一次
	if (ocr_engine_init == nullptr) {
		wchar_t old_path[512] = {};
		DWORD nlen = GetDllDirectoryW(512, old_path);

		::SetDllDirectoryW(engine.c_str());
		auto absdllName = engine + L"/" + dllName;
		auto hdll = LoadLibraryW(absdllName.c_str());
		
		
		if (hdll == NULL) {
			::SetDllDirectoryW(old_path);
			cout << "error: LoadLibraryA false:" << GetLastErrorAsString() << endl;
			return -1;
		}
		ocr_engine_init = (ocr_engine_init_t)GetProcAddress(hdll, "ocr_engine_init");
		ocr_engine_ocr = (ocr_engine_ocr_t)GetProcAddress(hdll, "ocr_engine_ocr");
		ocr_engine_release = (ocr_engine_release_t)GetProcAddress(hdll, "ocr_engine_release");
		if (ocr_engine_init && ocr_engine_ocr && ocr_engine_release) {}
		else {
			cout << "GetProcAddress false\n";
			ocr_engine_init = nullptr;
			::SetDllDirectoryW(old_path);
			return -2;
		}

	}
	if (m_engine == nullptr) {
		const int argc = argvs.size();
		char** argv=new char*[argc];
		//cout << "ocr_engine_init before\n";
		for (int i = 0; i < argc; i++) {
			argv[i] = new char[argvs[i].size() + 1];
			strcpy(argv[i], argvs[i].c_str());
			//cout << i << " new: " << "address:" << (void*)(argv[i]) << argv[i] << "\n";
		}
		ocr_engine_init(&m_engine, argv,argc);
		//cout << "ocr_engine_init after\n";
		for (int i = 0; i < argc; i++) {
			//cout <<i<< " delete: " << "address:"<<(void*)(argv[i]) << argv[i]<< "\n";
			delete []argv[i];
		}
		delete[]argv;
		//cout << " delete\n";
		//::SetDllDirectoryW(old_path);
		if (!m_engine) {
			cout << "ppaddle_init false\n";
			return -3;
		}
	}
	return 0;
}
int OcrWrapper::release() {
	if (m_engine) {
		ocr_engine_release(m_engine);
		m_engine = nullptr;
	}

	return 0;
}



int OcrWrapper::ocr(byte* data, int w, int h, int bpp, vocr_rec_t& result) {
	result.clear();
	using std::cout;
	using std::endl;
	if (m_engine == nullptr)return -1;
	ocr_engine_ocr_result* results;
	int n = 0;
	ocr_engine_ocr(m_engine, (void*)data, w, h, bpp, &results, &n);
	if (n > 0 && results) {
		for (int i = 0; i < n; i++) {
			auto& p = *(results + i);
			ocr_rec_t ts;
			ts.confidence = p.confidence;
			ts.left_top = point_t(p.x1, p.y1);
			ts.right_bottom = point_t(p.x2, p.y2);
			ts.text = _s2wstring(utf8_to_ansi(p.text));
			result.push_back(ts);
			free(results[i].text);

		}
		free(results);
	}
	
	return n;
}

