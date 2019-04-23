#include "hnet.h"
#include "args.h"
#include <iostream>
#include "OfsClient.h"

void start(int32_t argc, char ** argv) {
	args::ArgumentParser parser("This is a ofs client program.", "");
	args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
	args::ValueFlag<std::string> username(parser, "username", "username connect to ofs", { 'u', "user" }, "root");
	args::ValueFlag<std::string> password(parser, "password", "password connect to ofs", { 'p', "password" }, "");
	args::ValueFlag<std::string> host(parser, "host", "ofs host", { 'h', "host" }, "127.0.0.1");
	args::ValueFlag<int32_t> port(parser, "port", "ofs port", { 'P', "port" }, 15341);

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

	ofs::Client client;
	if (!client.Connect(args::get(host), args::get(port), args::get(username), args::get(password))) {
		std::cerr << "ofs server connect failed" << std::endl;
		return;
	}

	while (true) {
		client.Slash();

		std::string line;
		std::getline(std::cin, line);

		if (line == "quit" || line == "exit") {
			break;
		}

		client.DoCommand(line);
	}
}
