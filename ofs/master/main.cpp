#include "hnet.h"
#include "OfsMaster.h"
#include "args.h"
#include <iostream>

void start(int32_t argc, char ** argv) {
	args::ArgumentParser parser("This is a ofs master program.", "");
	args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
	args::ValueFlag<std::string> conf(parser, "configure", "The configure file path", { 'c', "config" });

	try
	{
		parser.ParseCLI(argc, argv);
	}
	catch (args::Help) {
		std::cout << parser;
		return;
	}
	catch (args::ParseError e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return;
	}
	catch (args::ValidationError e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return;
	}

	std::string path;
	if (conf)
		path = args::get(conf);
	else
#ifndef WIN32
		path = "/etc/ofs/master.conf";
#else
		path = "master.xml";
#endif

	if (!ofs::Master::Instance().Start(path)) {
		hn_error("master start failed");
		return;
	}

	hn_info("master start success");
}