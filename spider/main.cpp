#include "hnet.h"
#include "URLReader.h"
#include "URLGraph.h"
#include "HtmlParser.h"
#include <fstream>

void start(int32_t argc, char ** argv) {
	if (!UrlReader::Instance().Start(4))
		return;

	hn_channel<std::string, 20> tofile;
	hn_fork [tofile]{
		std::ofstream outfile("parsed.txt");
		while (true) {
			std::string out;
			tofile >> out;

			if (out == "##END##")
				break;

			outfile << out << std::endl;
		}

		outfile.flush();
		outfile.close();
	};

	UrlGraph::Instance().SetProccessor([tofile] (const std::string& url, const std::string& content) -> std::list<std::string> {
		//std::ofstream outfile("test.txt");
		//outfile << content;
		//outfile.flush();
		//outfile.close();

		printf("%lld:%s\n", content.size(), url.c_str());

		std::list<std::string> ret;
		auto doc = html_doc::HtmlParser().Parse(content);
		try {
			auto head = doc.Search("body h1.main-title");
			auto c = doc.Search("body div.article p");
			auto urls = doc.Search("body div.hot-news div.botData span.text a");
		
			std::string out = head.at(0)->InnerHtml();
			for (auto * t : c)
				out += "\n" +  t->InnerHtml();
			out += "\n";
			tofile << out;
			
			for (auto * t : urls) {
				std::string target = t->GetAttribute("href");
				if (target.find("mil.news.sina.com.cn/china/2019") != std::string::npos) {
					ret.push_back(std::move(target));
				}
			}
		}
		catch (std::exception& e) {
			printf("%s\n", e.what());
		}
		return ret;
	}).Search("https://mil.news.sina.com.cn/china/2019-04-08/doc-ihvhiewr3945199.shtml", 10, 4);

	tofile << "##END##";
}
