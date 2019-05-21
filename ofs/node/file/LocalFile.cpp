#include "LocalFile.h"
#include "api/OfsChunk.pb.h"
#include <fstream>

namespace ofs {
	int32_t LocalFile::Read(int32_t offset, int32_t size, std::string& data) {
		std::ifstream input(_path);
		if (!input)
			return api::chunk::ErrorCode::EC_BLOCK_FILE_NOT_EXIST;

		input.seekg(offset);
		if (!input)
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;
		
		data.resize(size);
		input.read((char*)data.c_str(), size);
		if (!input)
			return api::chunk::ErrorCode::EC_BLOCK_READ_FAILED;

		input.close();
		return api::chunk::ErrorCode::EC_NONE;
	}

	int32_t LocalFile::Write(int32_t offset, const char * data, int32_t size) {
		std::ofstream output(_path);
		if (!output)
			return api::chunk::ErrorCode::EC_BLOCK_OPEN_OR_CREATE_FILE_FAILED;

		output.seekp(offset);
		output.write(data, size);

		if (!output)
			return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;

		output.flush();
		output.close();
		return api::chunk::ErrorCode::EC_NONE;
	}
}