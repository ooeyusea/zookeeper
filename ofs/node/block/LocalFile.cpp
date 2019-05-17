#include "LocalFile.h"
#include "api/OfsChunk.pb.h"
#include <fstream>

namespace ofs {
	bool LocalFile::Read(int32_t offset, int32_t size, std::string& data) {
		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		std::ifstream input(_path);
		input.seekg(offset);
		if (!input)
			return false;
		
		data.resize(size);
		input.read((char*)data.c_str(), size);
		if (!input)
			return false;

		input.close();
		return true;
	}

	bool LocalFile::Write(int32_t offset, const char * data, int32_t size) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		std::ofstream output(_path);
		output.seekp(offset);
		output.write(data, size);
		
		if (!output)
			return false;

		output.flush();
		output.close();
		return true;
	}
}