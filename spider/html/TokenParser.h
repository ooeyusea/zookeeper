#ifndef __TOKEN_PARSER_H__
#define __TOKEN_PARSER_H__
#include "hnet.h"

namespace html_doc {
	struct TokenParserState {
		virtual ~TokenParserState() {}

		virtual TokenParserState * Poll(std::vector<std::string>& tokens, char c) = 0;
	};

	class TokenParser {
	public:
		TokenParser() {}
		~TokenParser() {
			if (_current) {
				delete _current;
				_current = nullptr;
			}
		}

		std::vector<std::string> Parse(const std::string& content);

	private:
		TokenParserState * _current = nullptr;
	};
}

#endif //__TOKEN_PARSER_H__
