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
	}

	TokenParserState * StateData::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '<') {
			tokens.push_back(_text);
			delete this;
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
			delete this;
			return new StateBeforeEndScript(std::move(_text));
		}
		return this;
	}

	TokenParserState * StateBeforeEndScript::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);

		if (c = '/') {
			delete this;
			return new StateEndScriptOpen(std::move(_text));
		}

		if (c == ' ' || c == '\t') {
			
		}
		else {
			delete this;
			return new StateScript(std::move(_text));
		}

		return this;
	}

	TokenParserState * StateEndScriptOpen::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);

		if (std::isalpha(c)) {
			_temp.push_back(std::tolower(c));

			delete this;
			return new StateEndScript(std::move(_text), std::move(_temp));
		}

		if (c == ' ' || c == '\t') {

		}
		else {
			delete this;
			return new StateScript(std::move(_text));
		}

		return this;
	}

	TokenParserState * StateEndScript::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);

		if (c == '>') {
			if (!CheckLastTag(tokens, _temp)) {
				delete this;
				return new StateScript(std::move(_text));
			}

			std::string::size_type pos = _text.find_last_of("<");
			_text.erase(pos);

			tokens.push_back(_text);
			tokens.push_back("<");
			tokens.push_back("/");
			tokens.push_back(_temp);
			tokens.push_back(">");

			delete this;
			return new StateData;
		}

		if (c == '<') {
			delete this;
			return new StateBeforeEndScript(std::move(_text));
		}

		if (c == ' ' || c == '\t') {
			delete this;
			return new StateEndScriptClose(std::move(_text), std::move(_temp));
		}

		if (std::isalpha(c)) {
			_temp.push_back(std::tolower(c));
		}
		else {
			delete this;
			return new StateScript(std::move(_text));
		}

		return this;
	}

	TokenParserState * StateEndScriptClose::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(c);
		if (c == '>') {
			if (!CheckLastTag(tokens, _temp)) {
				delete this;
				return new StateScript(std::move(_text));
			}

			std::string::size_type pos = _text.find_last_of("<");
			_text.erase(pos);

			tokens.push_back(_text);
			tokens.push_back("<");
			tokens.push_back("/");
			tokens.push_back(_temp);
			tokens.push_back(">");

			delete this;
			return new StateData;
		}

		if (c == '<') {
			delete this;
			return new StateBeforeEndScript(std::move(_text));
		}

		if (c == ' ' || c == '\t') {
		}
		else {
			delete this;
			return new StateScript(std::move(_text));
		}
		return this;
	}

	TokenParserState * StateBeforeEndScript::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '<') {
			tokens.push_back(_text);
			delete this;
			return new StateTagOpen;
		}
		else
			_text.push_back(c); //todo
		return this;
	}

	TokenParserState * StateTagOpen::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '!') {
			delete this;
			return new StateMarkupDeclarationOpen;
		}

		if (c == '/') {
			delete this;
			return new StateEndTagOpen;
		}

		if (std::isalpha(c)) {
			delete this;
			return new StateTagName(c);
		}
		return this;
	}

	TokenParserState * StateMarkupDeclarationOpen::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(std::tolower(c));
		if (_text == "--") {
			delete this;
			return new StateComment;
		}

		if (_text == "doctype") {
			delete this;
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
			delete this;
			return new StateData;
		}
		else
			_text.push_back(c);
		return this;
	}

	TokenParserState * StateComment::Poll(std::vector<std::string>& tokens, char c) {
		_text.push_back(std::tolower(c));
		if (_text.size() >= 2 && _text.at(_text.size() - 1) == '-' && _text.at(_text.size() - 2) == '-') {
			_text.pop_back();
			_text.pop_back();

			tokens.push_back("<");
			tokens.push_back("!");
			tokens.push_back("--");
			tokens.push_back(_text);
			tokens.push_back("--");
			tokens.push_back(">");
			delete this;
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
				delete this;
				return new StateScript;
			}
			else {
				delete this;
				return new StateData;
			}
		}

		if (c == '/') {
			tokens.push_back("<");
			tokens.push_back(_text);
			tokens.push_back("/");

			delete this;
			return new StateSelfEndTag;
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back("<");
			tokens.push_back(_text);

			if (_text == "script" || _text == "style") {
				delete this;
				return new StateScriptBeforeAttrName;
			}
			else {
				delete this;
				return new StateBeforeAttrName;
			}
		}

		_text.push_back(std::tolower(c));
		return this;
	}

	TokenParserState * StateBeforeAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			delete this;
			return new StateData;
		}

		if (c == '/') {
			tokens.push_back("/");

			delete this;
			return new StateSelfEndTag;
		}

		if (std::isalpha(c)) {
			delete this;
			return new StateAttrName(std::tolower(c));
		}

		return this;
	}


	TokenParserState * StateScriptBeforeAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			delete this;
			return new StateScript;
		}

		if (c == '/') {
			tokens.push_back("/");

			delete this;
			return new StateSelfEndTag;
		}

		if (std::isalpha(c)) {
			delete this;
			return new StateScriptAttrName(std::tolower(c));
		}

		return this;
	}

	TokenParserState * StateAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (std::isalpha(c) || c == '-' || std::isdigit(c)) {
			return this;
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back(_text);

			delete this;
			return new StateBeforeAttrValueEqual;
		}

		if (c == '=') {
			tokens.push_back(_text);
			tokens.push_back("=");

			delete this;
			return new StateBeforeAttrValue;
		}

		return this;
	}


	TokenParserState * StateScriptAttrName::Poll(std::vector<std::string>& tokens, char c) {
		if (std::isalpha(c) || c == '-' || std::isdigit(c)) {
			return this;
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back(_text);

			delete this;
			return new StateScriptBeforeAttrValueEqual;
		}

		if (c == '=') {
			tokens.push_back(_text);
			tokens.push_back("=");

			delete this;
			return new StateScriptBeforeAttrValue;
		}

		return this;
	}

	TokenParserState * StateBeforeAttrValueEqual::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '=') {
			tokens.push_back("=");

			delete this;
			return new StateBeforeAttrValue;
		}

		return this;
	}


	TokenParserState * StateScriptBeforeAttrValueEqual::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '=') {
			tokens.push_back("=");

			delete this;
			return new StateScriptBeforeAttrValue;
		}

		return this;
	}

	TokenParserState * StateBeforeAttrValue::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			delete this;
			return new StateAttrValueDoubleQuotes;
		}

		if (c == '\'') {
			delete this;
			return new StateAttrValueSingleQuotes;
		}

		return this;
	}

	TokenParserState * StateScriptBeforeAttrValue::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			delete this;
			return new StateScriptAttrValueDoubleQuotes;
		}

		if (c == '\'') {
			delete this;
			return new StateScriptAttrValueSingleQuotes;
		}

		return this;
	}

	TokenParserState * StateAttrValueDoubleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			tokens.push_back("\"");
			tokens.push_back(_text);
			tokens.push_back("\"");

			delete this;
			return new StateBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateScriptAttrValueDoubleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\"') {
			tokens.push_back("\"");
			tokens.push_back(_text);
			tokens.push_back("\"");

			delete this;
			return new StateScriptBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateAttrValueSingleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\'') {
			tokens.push_back("\'");
			tokens.push_back(_text);
			tokens.push_back("\'");

			delete this;
			return new StateBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateScriptAttrValueSingleQuotes::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '\'') {
			tokens.push_back("\'");
			tokens.push_back(_text);
			tokens.push_back("\'");

			delete this;
			return new StateScriptBeforeAttrName;
		}

		_text.push_back(c);
		return this;
	}

	TokenParserState * StateSelfEndTag::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			delete this;
			return new StateData;
		}

		return this;
	}

	TokenParserState * StateEndTagOpen::Poll(std::vector<std::string>& tokens, char c) {
		if (std::isalpha(c)) {
			delete this;
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

			delete this;
			return new StateData;
		}

		if (std::isalpha(c) || std::isdigit(c)) {
			_text.push_back(std::tolower(c));
		}

		if (c == ' ' || c == '\t') {
			tokens.push_back("<");
			tokens.push_back("/");
			tokens.push_back(_text);

			delete this;
			return new StateEndTagNameClose;
		}
		return this;
	}

	TokenParserState * StateEndTagNameClose::Poll(std::vector<std::string>& tokens, char c) {
		if (c == '>') {
			tokens.push_back(">");

			delete this;
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

		std::vector<std::string> ret;
		for (auto c : content)
			_current = _current->Poll(ret, c);
		
		return ret;
	}
}
