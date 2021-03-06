#ifndef __HTML_DOCUMENT_H__
#define __HTML_DOCUMENT_H__
#include "hnet.h"

namespace html_doc {
	inline std::vector<std::string> Split(std::string str, const char &sep) {
		std::vector<std::string> parts;

		std::string::size_type offset = 0;
		std::string::size_type pos = str.find(sep, offset);
		while (pos != std::string::npos) {
			parts.push_back(str.substr(offset, pos - offset));
			offset = pos + 1;

			pos = str.find(sep, offset);
		}

		parts.push_back(str.substr(offset));
		return parts;
	}

	class Query;
	class HtmlNode {
	public:
		HtmlNode() {}
		virtual ~HtmlNode() {}

		inline void SetParent(HtmlNode * parent) { _parent = parent; }
		
		virtual HtmlNode * Clone() = 0;
		virtual std::string InnerHtml() = 0;
		virtual std::string OuterHtml() = 0;

	protected:
		HtmlNode * _parent = nullptr;
	};

	class HtmlText : public HtmlNode {
	public:
		HtmlText() {}
		HtmlText(const std::string& text) : _text(text) {}
		HtmlText(std::string&& text) : _text(text) {}
		virtual ~HtmlText() {}

		virtual HtmlNode * Clone() { return new HtmlText(_text); }
		virtual std::string InnerHtml() { return _text; }
		virtual std::string OuterHtml() { return _text; }
		
	private:
		std::string _text;
	};

	class HtmlTag : public HtmlNode {
	public:
		HtmlTag() {}
		virtual ~HtmlTag() {}

		inline const std::string& GetId() const { return _id; }
		inline void SetId(const std::string& id) { _id = id; }
		inline void SetId(std::string&& id) { _id = id; }

		inline const std::string& GetName() const { return _name; }
		inline void SetName(const std::string& name) { _name = name; }
		inline void SetName(std::string&& name) { _name = name; }

		inline void AddClass(const std::string& name) { _classes.push_back(name); }
		inline void AddClass(std::string&& name) { _classes.push_back(name); }
		inline void SetClasses(std::vector<std::string>&& classes) { _classes = classes; }
		inline const std::vector<std::string>& GetClass() const { return _classes; }

		inline void AddAttribute(const std::string& key, std::string&& value) { _attributes[key] = value; }
		inline const std::unordered_map<std::string, std::string>& GetAttributes() const { return _attributes; }
		inline std::string GetAttribute(const std::string& key) const {
			auto itr = _attributes.find(key);
			if (itr != _attributes.end())
				return itr->second;
			return "";
		}

		inline void AddChild(HtmlNode * child) { _children.push_back(child); }
		inline const std::vector<HtmlNode *>& GetChildren() const { return _children; }

		inline bool HasCloseTag() const { return _hasCloseTag; }
		inline void SetCloseTag(bool val) { _hasCloseTag = val; }

		virtual HtmlNode * Clone();
		virtual std::string InnerHtml();
		virtual std::string OuterHtml();

	private:
		std::string _id;
		std::string _name;
		std::vector<std::string> _classes;
		std::unordered_map<std::string, std::string> _attributes;
		std::vector<HtmlNode *> _children;
		bool _hasCloseTag = false;
	};

	class HtmlDocument {
	public:
		HtmlDocument() {}
		HtmlDocument(HtmlNode * root) : _root(root) {}
		HtmlDocument(HtmlDocument&& doc) {
			std::swap(_root, doc._root);
		}
		HtmlDocument(const HtmlDocument& doc) {
			if (_root) {
				delete _root;
				_root = nullptr;
			}

			_root = doc._root->Clone();
		}

		~HtmlDocument() {
			if (_root) {
				delete _root;
				_root = nullptr;
			}
		}

		std::vector<HtmlTag*> Search(const std::string& q);

	private:
		HtmlNode * _root = nullptr;
	};
}

#endif //__HTML_DOCUMENT_H__
