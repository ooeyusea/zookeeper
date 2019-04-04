#include "TokenParser.h"

namespace html_doc {
	class DataState : public TokenParserState {
	public:
		DataState() {}
		virtual ~DataState() {}

		virtual TokenParserState * Poll(std::vector<std::string>& tokens, char c);

	private:
		std::string _text;
	};

	class TagOpenState : public TokenParserState {
	public:
		TagOpenState() {}
		virtual ~TagOpenState() {}

		virtual TokenParserState * Poll(std::vector<std::string>& tokens, char c);
	};

	TokenParserState * DataState::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '<') {
			delete this;
			return new TagOpenState;
		}
		else
			_text.push_back(c);
	}

	TokenParserState * TagOpenState::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '!') {
			delete this;
			return new MarkupDeclarationOpenState;
		}

		if (c == ' ' || c == '\t') {
			delete this;
			return new TagNameState;
		}
	}

	std::vector<std::string> TokenParser::parse(const std::string& content) {
		if (_current) {
			delete _current;
			_current = nullptr;
		}

		_current = new DataState;

		std::vector<std::string> ret;
		for (auto c : content)
			_current = _current->Poll(ret, c);
		
		return ret;
	}
}
