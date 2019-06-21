#include "LocalFile.h"
#include "api/OfsChunk.pb.h"
#include <fstream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace ofs {
	int32_t LocalFile::Read(int32_t offset, int32_t size, std::string& data) {
		std::ifstream input(_path, std::ios::binary);
		if (!input)
			return api::chunk::ErrorCode::EC_BLOCK_FILE_NOT_EXIST;

		input.seekg(offset);
		if (!input)
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;
		
		data.resize(size);
		input.read((char*)data.c_str(), size);
		if (!input || input.gcount() != size)
			return api::chunk::ErrorCode::EC_BLOCK_READ_FAILED;

		input.close();
		return api::chunk::ErrorCode::EC_NONE;
	}

	int32_t LocalFile::Write(int32_t offset, const char * data, int32_t size) {
		std::ofstream output(_path, std::ios::app | std::ios::binary);
		if (!output)
			return api::chunk::ErrorCode::EC_BLOCK_OPEN_OR_CREATE_FILE_FAILED;

		output.seekp(offset, std::ios::beg);
		output.write(data, size);

		if (!output)
			return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;

		output.flush();
		output.close();
		return api::chunk::ErrorCode::EC_NONE;
	}

	int32_t LocalFile::Append(const char * data, int32_t size) {
		std::ofstream output(_path, std::ios::app | std::ios::binary);
		if (!output)
			return api::chunk::ErrorCode::EC_BLOCK_OPEN_OR_CREATE_FILE_FAILED;

		output.write(data, size);

		if (!output)
			return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;

		output.flush();
		output.close();
		return api::chunk::ErrorCode::EC_NONE;
	}

	void LocalFile::Remove() {
		fs::remove(_path);
	}
}
