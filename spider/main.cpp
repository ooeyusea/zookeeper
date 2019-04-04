#include "hnet.h"
#include "URLReader.h"
#include "URLGraph.h"
#include "html_parser.h"
#include "html_node.h"
#include <fstream>

void start(int32_t argc, char ** argv) {
	if (!UrlReader::Instance().Start(4))
		return;

	UrlGraph::Instance().SetProccessor([] (const std::string& content) -> std::list<std::string> {
		//std::ofstream outfile("test.txt");
		//outfile << content;
		//outfile.flush();
		//outfile.close();

		std::list<std::string> ret;
		try {
			html_parser html;
			html.set_text(content);
			auto head = html.query("body h1").at(0);
			printf("aaaaa:%s\n", head->inner_html().c_str());
		}
		catch (std::exception& e) {
			printf("%s\n", e.what());
		}
		return ret;
	}).Search("https://mil.news.sina.com.cn/china/2019-04-03/doc-ihtxyzsm2775290.shtml", 10, 4);
}
