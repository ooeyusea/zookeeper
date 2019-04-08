#include "HtmlDocument.h"

namespace html_doc {
	HtmlNode * HtmlTag::Clone() {
		HtmlTag * tag = new HtmlTag;
		tag->SetName(_name);
		tag->SetCloseTag(_hasCloseTag);
		tag->_attributes = _attributes;
		tag->_classes = _classes;

		for (auto * child : _children) {
			auto * node = child->Clone();
			node->SetParent(tag);

			tag->AddChild(node);
		}

		return tag;
	}

	std::string HtmlTag::InnerHtml() {
		std::string text;
		for (auto * child : _children) {
			text += child->OuterHtml();
		}
		return text;
	}

	std::string HtmlTag::OuterHtml() {
		std::string text = "<" + _name;

		if (!_id.empty()) {
			text += " id=\"" + _id + "\"";
		}

		if (!_classes.empty()) {
			text += " class=\"";
			for (auto& c : _classes) {
				text += " " + c;
			}
			text += "\"";
		}

		for (auto itr = _attributes.begin(); itr != _attributes.end(); ++itr) {
			text += " " + itr->first + "=\"" + itr->second + "\"";
		}

		if (!_hasCloseTag && _children.empty()) {
			text += " />";
		}
		else {
			text += ">";
			for (auto * child : _children) {
				text += child->OuterHtml();
			}
			text += "</" + _name + ">";
		}

		return text;
	}

	class Query {
		struct QueryRule {
			std::string id;
			std::string tagName;
			std::vector<std::string> classes;
			bool isChild = false;
		};
	public:
		Query(const std::string& q) {
			std::vector<std::string> tokens = Split(q, ' ');

			for (auto& token : tokens) {
				QueryRule rule;

				std::string::size_type offset = 0;
				while (offset < token.size()) {
					char c = token.at(offset);
					if (c == '>') {
						++offset;

						_rules.push_back(rule);

						rule.isChild = true;
						rule.id.clear();
						rule.tagName.clear();
						rule.classes.clear();
						continue;
					}

					if (c == '.' ||  c == '#') {
						++offset;
					}

					std::string::size_type start = offset;
					for (; offset < token.size(); ++offset) {
						if (!std::isalnum(token.at(offset)) && token.at(offset) != '-' && token.at(offset) != '_') {
							if (c == '.')
								rule.classes.push_back(token.substr(start, offset - start));
							else if (c == '#')
								rule.id = token.substr(start, offset - start);
							else
								rule.tagName = token.substr(start, offset - start);
							break;
						}
					}

					if (offset == token.size()) {
						if (c == '.')
							rule.classes.push_back(token.substr(start, offset - start));
						else if (c == '#')
							rule.id = token.substr(start, offset - start);
						else
							rule.tagName = token.substr(start, offset - start);
					}
				}

				_rules.push_back(rule);
			}
		}

		inline std::vector<HtmlTag*> Search(HtmlNode * node) {
			std::vector<HtmlTag*> ret;

			HtmlTag * tag = dynamic_cast<HtmlTag*>(node);
			if (tag)
				Search(ret, tag, 0);

			return ret;
		}

	private:
		void Search(std::vector<HtmlTag*>& tags, HtmlTag * node, int32_t rule);
		bool Check(HtmlTag * tag, const QueryRule& r);

	private:
		std::vector<QueryRule> _rules;
	};

	void Query::Search(std::vector<HtmlTag*>& tags, HtmlTag * tag, int32_t rule) {
		if (rule >= _rules.size())
			return;

		//if (tag->GetName() == "h1") {
		//	int a = 0;
		//	++a;
		//}

		int32_t nextRule = rule;
		const QueryRule& r = _rules.at(rule);
		if (Check(tag, r)) {
			if (rule == _rules.size() - 1) {
				tags.push_back(tag);
				return;
			}
			++nextRule;
		}

		for (auto child : tag->GetChildren()) {
			auto t = dynamic_cast<HtmlTag*>(child);
			if (t)
				Search(tags, t, nextRule);
		}
	}

	bool Query::Check(HtmlTag * tag, const QueryRule& r) {
		if (!r.id.empty() && tag->GetId() != r.id)
			return false;

		if (!r.tagName.empty() && tag->GetName() != r.tagName)
			return false;

		if (!r.classes.empty()) {
			for (auto& c : r.classes) {
				if (std::none_of(tag->GetClass().begin(), tag->GetClass().end(), [c](const std::string& cls) {
					return c == cls;
				}))
					return false;
			}
		}

		return true;
	}

	std::vector<HtmlTag*> HtmlDocument::Search(const std::string& q) {
		if (_root) {
			Query query(q);

			return query.Search(_root);
		}
		
		return {};
	}
}
