#include "hnet.h"
#include "URLReader.h"

void start(int32_t argc, char ** argv) {
	if (!UrlReader::Instance().Start("conf.xml"))
		return;


}
