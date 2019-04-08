#include "TokenParser.h"

namespace html_doc {
#define TOKEN_STATE(name) class State##name : public TokenParserState {\
	public:\
		State##name() {}\
		State##name(char c) { _text.push_back(c); }\
		State##name(std::string&& text) { _text = text; }\
		State##name(std::string&& text, std::string&& temp) { _text = text; _temp = temp; }\
		virtual ~State##name() {}\
	\
	virtual TokenParserState * Poll(std::vector<std::string>& tokens, char c);\
	\
	private:\
		std::string _text;\
		std::string _temp; \
	}

	TOKEN_STATE(Data);
	TOKEN_STATE(TagOpen);
	TOKEN_STATE(MarkupDeclarationOpen);
	TOKEN_STATE(DocType);
	TOKEN_STATE(Comment);
	TOKEN_STATE(TagName);
	TOKEN_STATE(BeforeAttrName);
	TOKEN_STATE(AttrName);
	TOKEN_STATE(BeforeAttrValueEqual);
	TOKEN_STATE(BeforeAttrValue);
	TOKEN_STATE(AttrValueDoubleQuotes);
	TOKEN_STATE(AttrValueSingleQuotes);
	TOKEN_STATE(EndTagOpen);
	TOKEN_STATE(SelfEndTag);
	TOKEN_STATE(EndTagName);
	TOKEN_STATE(EndTagNameClose);
	TOKEN_STATE(Script);
	TOKEN_STATE(EndScriptOpen);
	TOKEN_STATE(EndScript);
	TOKEN_STATE(EndScriptClose);
	TOKEN_STATE(BeforeEndScript);
	TOKEN_STATE(ScriptBeforeAttrName);
	TOKEN_STATE(ScriptAttrName);
	TOKEN_STATE(ScriptBeforeAttrValueEqual);
	TOKEN_STATE(ScriptBeforeAttrValue);
	TOKEN_STATE(ScriptAttrValueDoubleQuotes);
	TOKEN_STATE(ScriptAttrValueSingleQuotes);

	bool CheckLastTag(const std::vector<std::string>& tokens, const std::string& tag) {
		for (auto itr = tokens.rbegin(); itr != tokens.rend(); ++itr) {
			if (*itr == "<" && itr != tokens.rbegin()) {
				auto tagItr = itr - 1;
				return tag == *tagItr;
			}
		}
		return false;
	}

	TokenParserState * StateData::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '<') {
			tokens.push_back(_text);
			return new StateTagOpen;
		}
		else if (c == '\r' || c == '\n') {

		}
		else
			_text.push_back(c);
		return this;
	}

	TokenParserState * StateScript::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);

		if (c == '<') {
			return new StateBeforeEndScript(std::move(_text));
		}
		return this;
	}

	TokenParserState * StateBeforeEndScript::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);

		if (c = '/') {
			return new StateEndScriptOpen(std::move(_text));
		}

		if (c == ' ' || c == '\t') {
			
		}
		else {
			return new StateScript(std::move(_text));
		}

		return this;
	}

	TokenParserState * StateEndScriptOpen::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);

		if (std::isalpha(c)) {
			_temp.push_back(std::tolower(c));

			return new StateEndScript(std::move(_text), std::move(_temp));
		}

		if (c == ' ' || c == '\t') {

		}
		else {
			return new StateScript(std::move(_text));
		}

		return this;
	}

	TokenParserState * StateEndScript::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);

		if (c == '>') {
			if (!CheckLastTag(tokens, _temp)) {
				return new StateScript(std::move(_text));
			}

			std::string::size_type pos = _text.find_last_of("<");
			_text.erase(pos);

			tokens.push_back(_text);
			tokens.push_back("<");
			tokens.push_back("/");
			tokens.push_back(_temp);
			tokens.push_back(">");

			return new StateData;
		}

		if (c == '<') {
			return new StateBeforeEndScript(std::move(_text));
		}

		if (c == ' ' || c == '\t') {
			return new StateEndScriptClose(std::move(_text), std::move(_temp));
		}

		if (std::isalpha(c)) {
			_temp.push_back(std::tolower(c));
		}
		else {
			return new StateScript(std::move(_text));
		}

		return this;
	}

	TokenParserState * StateEndScriptClose::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);
		if (c == '>') {
			if (!CheckLastTag(tokens, _temp)) {
				return new StateScript(std::move(_text));
			}

			std::string::size_type pos = _text.find_last_of("<");
			_text.erase(pos);

			tokens.push_back(_text);
			tokens.push_back("<");
			tokens.push_back("/");
			tokens.push_back(_temp);
			tokens.push_back(">");

			return new StateData;
		}

		if (c == '<') {
			return new StateBeforeEndScript(std::move(_text));
		}

		if (c == ' ' || c == '\t') {
		}
		else {
			return new StateScript(std::move(_text));
		}
		return this;
	}

	TokenParserState * StateTagOpen::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '!') {
			return new StateMarkupDeclarationOpen;
		}

		if (c == '/') {
			return new StateEndTagOpen;
		}

		if (std::isalpha(c)) {
			return new StateTagName(c);
		}
		return this;
	}

	TokenParserState * StateMarkupDeclarationOpen::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(std::tolower(c));
		if (_text == "--") {
			return new StateComment;
		}

		if (_text == "doctype") {
			return new StateDocType;
		}

		return this;
	}

	TokenParserState * StateDocType::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back("<");
			tokens.push_back("!");
			tokens.push_back("doctype");
			tokens.push_back(_text);
			tokens.push_back(">");
			return new StateData;
		}
		else
			_text.push_back(c);
		return this;
	}

	TokenParserState * StateComment::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(std::tolower(c));
		if (_text.size() >= 3 && _text.at(_text.size() - 1) == '>' && _text.at(_text.size() - 2) == '-' && _text.at(_text.size() - 3) == '-') {
			_text.pop_back();
			_text.pop_back();
			_text.pop_back();

			tokens.push_back("<");
			tokens.push_back("!");
			tokens.push_back("--");
			tokens.push_back(_text);
			tokens.push_back("--");
			tokens.push_back(">");
			return new StateData;
		}
		return this;
	}

	TokenParserState * StateTagName::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back("<");
			tokens.push_back(_text);
			tokens.push_back(">");

			if (_text == "script" || _text == "style") {
				return new StateScript;
			}
			else {
				return new StateData;
			}
		}

		if (c == '/') {
			tokens.push_back("<");
			tokens.push_back(_text);
			tokens.push_back("/");

			return new StateSelfEndTag;
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back("<");
			tokens.push_back(_text);

			if (_text == "script" || _text == "style") {
				return new StateScriptBeforeAttrName;
			}
			else {
				return new StateBeforeAttrName;
			}
		}

		_text.push_back(std::tolower(c));
		return this;
	}

	TokenParserState * StateBeforeAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			return new StateData;
		}

		if (c == '/') {
			tokens.push_back("/");

			return new StateSelfEndTag;
		}

		if (std::isalpha(c)) {
			return new StateAttrName(std::tolower(c));
		}

		return this;
	}


	TokenParserState * StateScriptBeforeAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			return new StateScript;
		}

		if (c == '/') {
			tokens.push_back("/");

			return new StateSelfEndTag;
		}

		if (std::isalpha(c)) {
			return new StateScriptAttrName(std::tolower(c));
		}

		return this;
	}

	TokenParserState * StateAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (std::isalpha(c) || c == '-' || std::isdigit(c)) {
			_text.push_back(std::tolower(c));
			return this;
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back(_text);

			return new StateBeforeAttrValueEqual;
		}

		if (c == '=') {
			tokens.push_back(_text);
			tokens.push_back("=");

			return new StateBeforeAttrValue;
		}

		return this;
	}


	TokenParserState * StateScriptAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (std::isalpha(c) || c == '-' || std::isdigit(c)) {
			_text.push_back(std::tolower(c));
			return this;
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back(_text);

			return new StateScriptBeforeAttrValueEqual;
		}

		if (c == '=') {
			tokens.push_back(_text);
			tokens.push_back("=");

			return new StateScriptBeforeAttrValue;
		}

		return this;
	}

	TokenParserState * StateBeforeAttrValueEqual::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '=') {
			tokens.push_back("=");

			return new StateBeforeAttrValue;
		}

		return this;
	}


	TokenParserState * StateScriptBeforeAttrValueEqual::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '=') {
			tokens.push_back("=");

			return new StateScriptBeforeAttrValue;
		}

		return this;
	}

	TokenParserState * StateBeforeAttrValue::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			return new StateAttrValueDoubleQuotes;
		}

		if (c == '\'') {
			return new StateAttrValueSingleQuotes;
		}

		if (c == '>') {
			tokens.push_back("");
			tokens.push_back(">");

			return new StateData;
		}

		return this;
	}

	TokenParserState * StateScriptBeforeAttrValue::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			return new StateScriptAttrValueDoubleQuotes;
		}

		if (c == '\'') {
			return new StateScriptAttrValueSingleQuotes;
		}

		if (c == '>') {
			tokens.push_back("");
			tokens.push_back(">");

			return new StateData;
		}

		return this;
	}

	TokenParserState * StateAttrValueDoubleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			tokens.push_back(_text);

			return new StateBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateScriptAttrValueDoubleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			tokens.push_back(_text);

			return new StateScriptBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateAttrValueSingleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\'') {
			tokens.push_back(_text);

			return new StateBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateScriptAttrValueSingleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\'') {
			tokens.push_back(_text);

			return new StateScriptBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateSelfEndTag::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			return new StateData;
		}

		return this;
	}

	TokenParserState * StateEndTagOpen::Poll(std::vector<std::string>& tokens, char c) {
		if (std::isalpha(c)) {
			return new StateEndTagName(c);
		}

		return this;
	}

	TokenParserState * StateEndTagName::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back("<");
			tokens.push_back("/");
			tokens.push_back(_text);
			tokens.push_back(">");

			return new StateData;
		}

		if (std::isalpha(c) || std::isdigit(c)) {
			_text.push_back(std::tolower(c));
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back("<");
			tokens.push_back("/");
			tokens.push_back(_text);

			return new StateEndTagNameClose;
		}
		return this;
	}

	TokenParserState * StateEndTagNameClose::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			return new StateData;
		}

		return this;
	}

	std::vector<std::string> TokenParser::Parse(const std::string& content) {
		if (_current) {
			delete _current;
			_current = nullptr;
		}

		_current = new StateData;

		int32_t i = 0;
		std::vector<std::string> ret;
		for (auto c : content) {
			auto * next = _current->Poll(ret, c);
			if (next != _current) {
				delete _current;
				_current = next;
			}
			++i;
		}
		
		return ret;
	}
}
