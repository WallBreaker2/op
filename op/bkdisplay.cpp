#include "stdafx.h"
#include "bkdisplay.h"
#include "Tool.h"

bkdisplay::bkdisplay()
{
	_hwnd = NULL;
	_region = nullptr;
	_pmutex = nullptr;
	_bind_state = 0;
	_width = _height = 0;
}


bkdisplay::~bkdisplay()
{
}

long bkdisplay::bind_init() {
	sprintf(_shared_res_name, SHARED_RES_NAME_FORMAT, _hwnd);
	sprintf(_mutex_name, MUTEX_NAME_FORMAT, _hwnd);
	try {
		boost::interprocess::shared_memory_object shm(
			boost::interprocess::create_only,
			_shared_res_name,
			boost::interprocess::read_write);
		shm.truncate(SHARED_MEMORY_SIZE);
		_region = new boost::interprocess::mapped_region(shm, boost::interprocess::read_only);
		//_image_data = (byte*)_region->get_address();
		_pmutex = new boost::interprocess::named_mutex(boost::interprocess::create_only, _mutex_name);
	}
	catch (std::exception&e) {
		Tool::setlog("Bkdx::bind_init %s exception:%s", _shared_res_name, e.what());
	}

	return 0;
}

long bkdisplay::bind_release() {
	try {
		using namespace boost::interprocess;
		shared_memory_object::remove(_shared_res_name);
		named_mutex::remove(_mutex_name);
		SAFE_DELETE(_region);
		SAFE_DELETE(_pmutex);

	}
	catch (std::exception&e) {
		Tool::setlog("Bkdx::bind_release std::exception:%s", e.what());
	}

	//_image_data = nullptr;
	return 0;
}

