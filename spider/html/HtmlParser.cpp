#include "HtmlParser.h"
#include "TokenParser.h"
#include <stack>

namespace html_doc {
	void LTrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			if (ch < 0)
				return true;
			return !::isspace(ch);
		}));
	}

	void RTrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			if (ch < 0)
				return true;
			return !::isspace(ch);
		}).base(), s.end());
	}

	std::string& Trim(std::string &s) {
		LTrim(s);
		RTrim(s);

		return s;
	}

	int32_t Find(const std::vector<std::string>& tokens, int32_t offset, const std::string& c) {
		for (int32_t i = offset; i < (int32_t)tokens.size(); ++i) {
			if (tokens[i] == c)
				return i;
		}

		return -1;
	}

	int32_t SkipDoctype(const std::vector<std::string>& tokens, int32_t offset) {
		auto pos = Find(tokens, offset, "<");
		if (pos != -1)
			if (pos + 4 < (int32_t)tokens.size() && tokens[1] == "!" && tokens[2] == "doctype" && tokens[4] == ">")
				return pos + 5;

		return offset;
	}


	bool SkipAnnotation(std::vector<std::string> &tokens, int32_t &i) {
		int32_t j = i;
		if (tokens.size() > j + 3) { //<!-- -->
			if (strncmp(tokens.at(j + 2).c_str(), "--", 2) == 0) {
				j = j + 3;
				while (j < tokens.size()) {
					if (tokens.at(j) == ">") {
						if (tokens.at(j - 1).size() >= 2 && tokens.at(j - 1).substr(tokens.at(j - 1).size() - 2, 2) == "--") {
							i = j;
							return true;
						}
					}
					++j;
				}
			}
		}
		return false;
	}


	void ReadTextNodeUntil(HtmlTag * tag, std::vector<std::string> &tokens, int32_t &i) {
		std::string text;

		int32_t j = i + 1;
		while (j < tokens.size()) {
			if (tokens.at(j) == "<") {
				if (j + 3 < tokens.size()) {
					if (tokens.at(j + 1) == "/" && tokens.at(j + 2) == tag->GetName() && tokens.at(j + 3) == ">") {
						i = j + 3;

						if (!text.empty()) {
							HtmlText *textNode = new HtmlText(text);
							textNode->SetParent(tag);
							tag->AddChild(textNode);
						}
						tag->SetCloseTag(false);
						return;
					}
				}
			}

			text.append(tokens.at(j));
			++j;
		}
	}


	HtmlTag * ParseTagBegin(std::vector<std::string>& tokens, int32_t &i) {
		i++;

		HtmlTag * tag = new HtmlTag;
		tag->SetName(tokens[i++]);

		std::string  name, value;
		int step = 0;
		if (tokens.at(i) != ">")
			while (true) {
				if (i == tokens.size()) {
					delete tag;
					throw std::logic_error("Unexpected end of document");
				}

				auto& token = tokens.at(i++);

				if (token == ">")
					break;

				switch (step) {
				case 0:
					name = token;
					step++;
					break;

				case 1:
					if (token == "=")
						step++;
					else {
						delete tag;
						throw std::logic_error("Unexpected " + token + " token");
					}
					break;

				case 2:
					step = 0;
					value = token;
					if (name == "id")
						tag->SetId(std::move(value));
					else if (name == "class") {
						std::vector<std::string> classes = Split(value, ' ');
						tag->SetClasses(std::move(classes));
					}
					else
						tag->AddAttribute(name, std::move(value));
					break;
				}
			}

			tag->SetCloseTag(tokens.at(i - 2) != "/");
			if (tag->GetName() == "img" || tag->GetName() == "meta" || tag->GetName() == "link" || tag->GetName() == "input" || tag->GetName() == "br" || tag->GetName() == "hr")
				tag->SetCloseTag(false);

			i--;
			return tag;
	}

	HtmlDocument HtmlParser::Parse(const std::string& content) {
		std::vector<std::string> tokens = TokenParser().Parse(content);

		std::stack<HtmlTag*> stack;
		HtmlTag * root = new HtmlTag;
		stack.push(root);
		
		for (int32_t i = SkipDoctype(tokens, 0); i < (int32_t)tokens.size(); ++i) {
			std::string token = tokens.at(i);

			if (all_of(token.begin(), token.end(),
				[](char n) {return !iswprint(n); }))
				continue;

			if (token == "<") {
				if (tokens.size() <= i + 1)
					throw std::logic_error("Unexpected end of document");

				if (tokens.at(i + 1) == "!") {
					if (SkipAnnotation(tokens, i))
						continue;
				}
				else if (tokens.at(i + 1) == "/") {
					stack.pop();
				}
				else {
					HtmlTag * tag = ParseTagBegin(tokens, i);
					if (tag) {
						if (stack.size()) {
							tag->SetParent(stack.top());
							stack.top()->AddChild(tag);
						}

						if (tag->HasCloseTag())
							stack.push(tag);
					}
				}
			}

			token = tokens.at(i);
			if (token == ">") {
				if (!stack.empty()) {
					if (stack.top()->GetName() == "script" || stack.top()->GetName() == "style") {
						ReadTextNodeUntil(stack.top(), tokens, i);
						stack.pop();
						continue;
					}
				}

				if (tokens.size() > i + 1 && tokens.at(i + 1) != "<") {
					auto& text = Trim(tokens.at(i + 1));
					if (std::any_of(text.begin(), text.end(), &iswalpha)) {
						HtmlText *textNode = new HtmlText(text);
						textNode->SetParent(stack.top());
						stack.top()->AddChild(textNode);
					}
				}
			}
		}

		return HtmlDocument(root);
	}
}
