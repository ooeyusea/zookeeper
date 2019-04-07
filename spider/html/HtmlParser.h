#ifndef __HTML_PARSER_H__
#define __HTML_PARSER_H__
#include "HtmlParser.h"
#include "HtmlDocument.h"

namespace html_doc {
	class HtmlParser {
	public:
		HtmlParser() {}
		~HtmlParser() {}

		HtmlDocument Parse(const std::string& content);
	private:
	};
}

#endif //__HTML_PARSER_H__
