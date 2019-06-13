#pragma once
class query_api
{
public:
	query_api();
	~query_api();
	template<typename func_t>
	func_t query(const char* mod_name, const char* func_name) {
		auto hdll = ::GetModuleHandleA(mod_name);
		if (!hdll) {
			_error_code = -1;
			return NULL;
		}
		void* paddress = (void*)::GetProcAddress(hdll, func_name);
		if (!paddress) {
			_error_code = -2;
			return NULL;
		}
		_error_code = 0;
		return (func_t)paddress;

	}
	int error_code() {
		return _error_code;
	}
private:
	int _error_code;
};

