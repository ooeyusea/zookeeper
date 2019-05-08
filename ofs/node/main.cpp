#include "hnet.h"
#include "OfsNode.h"
#include "args.h"
#include <iostream>

void start(int32_t argc, char ** argv) {
	args::ArgumentParser parser("This is a ofs node program.", "");
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
		path = "/etc/ofs/node.conf";
#else
		path = "conf.xml";
#endif

	if (!ofs::Node::Instance().Start(path)) {
		hn_error("node start failed");
		return;
	}

	hn_info("node start success");
}