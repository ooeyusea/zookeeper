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

	class Query {
		struct QueryRule {
			std::string id;
			std::string tagName;
			std::vector<std::string> classes;
			bool is_child;
		};
	public:
		Query(const std::string& q) {
			std::vector<std::string> tokens = Split(q, ' ');

			QueryRule rule;
			for (auto& token : tokens) {
				if (token == ".") {
					rule.classes.push_back(_tokens.at(i));
					continue;
				}
				if (t == "#") {
					++i;
					last_rule->id = _tokens.at(i);
					continue;
				}
				if (t == ">") {
					rl.push_back(last_rule);
					last_rule = new query_rule_t;
					last_rule->is_child = true;
					continue;
				}

				if (last_rule->is_valid())
					rl.push_back(last_rule);
				last_rule = new query_rule_t;
				last_rule->is_child = false;
				last_rule->tag_name = t;
				rules.push_back(rl);
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
