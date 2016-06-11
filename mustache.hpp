//
// Copyright 2015-2016 Kevin Wojniak
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#ifndef KAINJOW_MUSTACHE_HPP
#define KAINJOW_MUSTACHE_HPP

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Kainjow {

template <typename StringType>
StringType trim(const StringType& s) {
    auto it = s.begin();
    while (it != s.end() && isspace(*it)) {
        it++;
    }
    auto rit = s.rbegin();
    while (rit.base() != it && isspace(*rit)) {
        rit++;
    }
    return {it, rit.base()};
}

template <typename StringType>
StringType escape(const StringType& s) {
    StringType ret;
    ret.reserve(s.size()*2);
    for (const auto ch : s) {
        switch (ch) {
            case '&':
                ret.append({'&','a','m','p',';'});
                break;
            case '<':
                ret.append({'&','l','t',';'});
                break;
            case '>':
                ret.append({'&','g','t',';'});
                break;
            case '\"':
                ret.append({'&','q','u','o','t',';'});
                break;
            case '\'':
                ret.append({'&','a','p','o','s',';'});
                break;
            default:
                ret.append(1, ch);
                break;
        }
    }
    return ret;
}

template <typename StringType>
class BasicMustache {
public:
    
    class Data {
    public:
        enum class Type {
            Object,
            String,
            List,
            True,
            False,
            Partial,
            Lambda,
            Invalid,
        };
        
        using ObjectType = std::unordered_map<StringType, Data>;
        using ListType = std::vector<Data>;
        using PartialType = std::function<StringType()>;
        using LambdaType = std::function<StringType(const StringType&)>;
        
        // Construction
        Data() : Data(Type::Object) {
        }
        Data(const StringType& string) : type_{Type::String} {
            str_.reset(new StringType(string));
        }
        Data(const typename StringType::value_type* string) : type_{Type::String} {
            str_.reset(new StringType(string));
        }
        Data(const ListType& list) : type_{Type::List} {
            list_.reset(new ListType(list));
        }
        Data(Type type) : type_{type} {
            switch (type_) {
                case Type::Object:
                    obj_.reset(new ObjectType);
                    break;
                case Type::String:
                    str_.reset(new StringType);
                    break;
                case Type::List:
                    list_.reset(new ListType);
                    break;
                default:
                    break;
            }
        }
        Data(const StringType& name, const Data& var) : Data{} {
            set(name, var);
        }
        Data(const PartialType& partial) : type_{Type::Partial} {
            partial_.reset(new PartialType(partial));
        }
        Data(const LambdaType& lambda) : type_{Type::Lambda} {
            lambda_.reset(new LambdaType(lambda));
        }
        static Data List() {
            return {Data::Type::List};
        }
        
        // Copying
        Data(const Data& data) : type_(data.type_) {
            if (data.obj_) {
                obj_.reset(new ObjectType(*data.obj_));
            } else if (data.str_) {
                str_.reset(new StringType(*data.str_));
            } else if (data.list_) {
                list_.reset(new ListType(*data.list_));
            } else if (data.partial_) {
                partial_.reset(new PartialType(*data.partial_));
            } else if (data.lambda_) {
                lambda_.reset(new LambdaType(*data.lambda_));
            }
        }
        
        // Move
        Data(Data&& data) : type_{data.type_} {
            if (data.obj_) {
                obj_ = std::move(data.obj_);
            } else if (data.str_) {
                str_ = std::move(data.str_);
            } else if (data.list_) {
                list_ = std::move(data.list_);
            } else if (data.partial_) {
                partial_ = std::move(data.partial_);
            } else if (data.lambda_) {
                lambda_ = std::move(data.lambda_);
            }
            data.type_ = Data::Type::Invalid;
        }
        Data& operator= (Data&& data) {
            if (this != &data) {
                obj_.reset();
                str_.reset();
                list_.reset();
                partial_.reset();
                lambda_.reset();
                if (data.obj_) {
                    obj_ = std::move(data.obj_);
                } else if (data.str_) {
                    str_ = std::move(data.str_);
                } else if (data.list_) {
                    list_ = std::move(data.list_);
                } else if (data.partial_) {
                    partial_ = std::move(data.partial_);
                } else if (data.lambda_) {
                    lambda_ = std::move(data.lambda_);
                }
                type_ = data.type_;
                data.type_ = Data::Type::Invalid;
            }
            return *this;
        }
        
        // Type info
        Type type() const {
            return type_;
        }
        bool isObject() const {
            return type_ == Type::Object;
        }
        bool isString() const {
            return type_ == Type::String;
        }
        bool isList() const {
            return type_ == Type::List;
        }
        bool isBool() const {
            return type_ == Type::True || type_ == Type::False;
        }
        bool isTrue() const {
            return type_ == Type::True;
        }
        bool isFalse() const {
            return type_ == Type::False;
        }
        bool isPartial() const {
            return type_ == Type::Partial;
        }
        bool isLambda() const {
            return type_ == Type::Lambda;
        }
        
        // Object data
        void set(const StringType& name, const Data& var) {
            if (isObject()) {
                obj_->insert(std::pair<StringType,Data>{name, var});
            }
        }
        const Data* get(const StringType& name) const {
            if (!isObject()) {
                return nullptr;
            }
            const auto& it = obj_->find(name);
            if (it == obj_->end()) {
                return nullptr;
            }
            return &it->second;
        }
        
        // List data
        void push_back(const Data& var) {
            if (isList()) {
                list_->push_back(var);
            }
        }
        const ListType& list() const {
            return *list_;
        }
        bool isEmptyList() const {
            return isList() && list_->empty();
        }
        bool isNonEmptyList() const {
            return isList() && !list_->empty();
        }
        Data& operator<< (const Data& data) {
            push_back(data);
            return *this;
        }
        
        // String data
        const StringType& stringValue() const {
            return *str_;
        }
        
        Data& operator[] (const StringType& key) {
            return (*obj_)[key];
        }
        
        const PartialType& partial() const {
            return (*partial_);
        }
        
        const LambdaType& lambda() const {
            return (*lambda_);
        }
        
        Data callLambda(const StringType& text) const {
            return (*lambda_)(text);
        }
        
    private:
        Type type_;
        std::unique_ptr<ObjectType> obj_;
        std::unique_ptr<StringType> str_;
        std::unique_ptr<ListType> list_;
        std::unique_ptr<PartialType> partial_;
        std::unique_ptr<LambdaType> lambda_;
    };
    
    BasicMustache(const StringType& input) {
        Context ctx;
        parse(input, ctx);
    }
    
    bool isValid() const {
        return errorMessage_.empty();
    }
    
    const StringType& errorMessage() const {
        return errorMessage_;
    }

    template <typename StreamType>
    StreamType& render(const Data& data, StreamType& stream) {
        render(data, [&stream](const StringType& str) {
            stream << str;
        });
        return stream;
    }
    
    StringType render(const Data& data) {
        std::basic_ostringstream<typename StringType::value_type> ss;
        return render(data, ss).str();
    }

    using RenderHandler = std::function<void(const StringType&)>;
    void render(const Data& data, const RenderHandler& handler) {
        Context ctx{&data};
        render(handler, ctx);
    }

private:
    using StringSizeType = typename StringType::size_type;
    
    class DelimiterSet {
    public:
        StringType begin;
        StringType end;
        DelimiterSet() {
            reset();
        }
        bool isDefault() const { return begin == defaultBegin() && end == defaultEnd(); }
        void reset() {
            begin = defaultBegin();
            end = defaultEnd();
        }
        static StringType defaultBegin() {
            return StringType(2, '{');
        }
        static StringType defaultEnd() {
            return StringType(2, '}');
        }
    };
    
    class Tag {
    public:
        enum class Type {
            Invalid,
            Variable,
            UnescapedVariable,
            SectionBegin,
            SectionEnd,
            SectionBeginInverted,
            Comment,
            Partial,
            SetDelimiter,
        };
        StringType name;
        Type type = Type::Invalid;
        std::shared_ptr<StringType> sectionText;
        std::shared_ptr<DelimiterSet> delimiterSet;
        bool isSectionBegin() const {
            return type == Type::SectionBegin || type == Type::SectionBeginInverted;
        }
        bool isSectionEnd() const {
            return type == Type::SectionEnd;
        }
    };
    
    class Component {
    public:
        StringType text;
        Tag tag;
        std::vector<Component> children;
        StringSizeType position = StringType::npos;
        bool isText() const {
            return tag.type == Tag::Type::Invalid;
        }
        Component() {}
        Component(const StringType& t, StringSizeType p) : text(t), position(p) {}
    };
    
    class Context {
    public:
        Context(const Data* data) {
            push(data);
        }

        Context() {
        }

        void push(const Data* data) {
            items_.insert(items_.begin(), data);
        }

        void pop() {
            items_.erase(items_.begin());
        }
        
        static std::vector<StringType> split(const StringType& s, typename StringType::value_type delim) {
            std::vector<StringType> elems;
            std::basic_stringstream<typename StringType::value_type> ss(s);
            StringType item;
            while (std::getline(ss, item, delim)) {
                elems.push_back(item);
            }
            return elems;
        }

        const Data* get(const StringType& name) const {
            // process {{.}} name
            if (name.size() == 1 && name.at(0) == '.') {
                return items_.front();
            }
            // process normal name
            auto names = split(name, '.');
            if (names.size() == 0) {
                names.resize(1);
            }
            for (const auto& item : items_) {
                const Data* var{item};
                for (const auto& n : names) {
                    var = var->get(n);
                    if (!var) {
                        break;
                    }
                }
                if (var) {
                    return var;
                }
            }
            return nullptr;
        }

        const Data* get_partial(const StringType& name) const {
            for (const auto& item : items_) {
                const Data* var{item};
                var = var->get(name);
                if (var) {
                    return var;
                }
            }
            return nullptr;
        }

        Context(const Context&) = delete;
        Context& operator= (const Context&) = delete;
        
        DelimiterSet delimiterSet;

    private:
        std::vector<const Data*> items_;
    };

    class ContextPusher {
    public:
        ContextPusher(Context& ctx, const Data* data) : ctx_(ctx) {
            ctx.push(data);
        }
        ~ContextPusher() {
            ctx_.pop();
        }
        ContextPusher(const ContextPusher&) = delete;
        ContextPusher& operator= (const ContextPusher&) = delete;
    private:
        Context& ctx_;
    };
    
    BasicMustache(const StringType& input, Context& ctx) {
        parse(input, ctx);
    }

    void parse(const StringType& input, Context& ctx) {
        using streamstring = std::basic_ostringstream<typename StringType::value_type>;
        
        const StringType braceDelimiterEndUnescaped(3, '}');
        const StringSizeType inputSize{input.size()};
        
        bool currentDelimiterIsBrace{ctx.delimiterSet.isDefault()};
        
        std::vector<Component*> sections{&rootComponent_};
        std::vector<StringSizeType> sectionStarts;
        
        StringSizeType inputPosition{0};
        while (inputPosition != inputSize) {
            
            // Find the next tag start delimiter
            const StringSizeType tagLocationStart{input.find(ctx.delimiterSet.begin, inputPosition)};
            if (tagLocationStart == StringType::npos) {
                // No tag found. Add the remaining text.
                const Component comp{{input, inputPosition, inputSize - inputPosition}, inputPosition};
                sections.back()->children.push_back(comp);
                break;
            } else if (tagLocationStart != inputPosition) {
                // Tag found, add text up to this tag.
                const Component comp{{input, inputPosition, tagLocationStart - inputPosition}, inputPosition};
                sections.back()->children.push_back(comp);
            }
            
            // Find the next tag end delimiter
            StringSizeType tagContentsLocation{tagLocationStart + ctx.delimiterSet.begin.size()};
            const bool tagIsUnescapedVar{currentDelimiterIsBrace && tagLocationStart != (inputSize - 2) && input.at(tagContentsLocation) == ctx.delimiterSet.begin.at(0)};
            const StringType& currentTagDelimiterEnd{tagIsUnescapedVar ? braceDelimiterEndUnescaped : ctx.delimiterSet.end};
            const auto currentTagDelimiterEndSize = currentTagDelimiterEnd.size();
            if (tagIsUnescapedVar) {
                ++tagContentsLocation;
            }
            StringSizeType tagLocationEnd{input.find(currentTagDelimiterEnd, tagContentsLocation)};
            if (tagLocationEnd == StringType::npos) {
                streamstring ss;
                ss << "Unclosed tag at " << tagLocationStart;
                errorMessage_.assign(ss.str());
                return;
            }
            
            // Parse tag
            const StringType tagContents{trim(StringType{input, tagContentsLocation, tagLocationEnd - tagContentsLocation})};
            Component comp;
            if (!tagContents.empty() && tagContents[0] == '=') {
                if (!parseSetDelimiterTag(tagContents, ctx.delimiterSet)) {
                    streamstring ss;
                    ss << "Invalid set delimiter tag at " << tagLocationStart;
                    errorMessage_.assign(ss.str());
                    return;
                }
                currentDelimiterIsBrace = ctx.delimiterSet.isDefault();
                comp.tag.type = Tag::Type::SetDelimiter;
                comp.tag.delimiterSet.reset(new DelimiterSet(ctx.delimiterSet));
            }
            if (comp.tag.type != Tag::Type::SetDelimiter) {
                parseTagContents(tagIsUnescapedVar, tagContents, comp.tag);
            }
            comp.position = tagLocationStart;
            sections.back()->children.push_back(comp);
            
            // Start next search after this tag
            inputPosition = tagLocationEnd + currentTagDelimiterEndSize;

            // Push or pop sections
            if (comp.tag.isSectionBegin()) {
                sections.push_back(&sections.back()->children.back());
                sectionStarts.push_back(inputPosition);
            } else if (comp.tag.isSectionEnd()) {
                if (sections.size() == 1) {
                    streamstring ss;
                    ss << "Unopened section \"" << comp.tag.name << "\" at " << comp.position;
                    errorMessage_.assign(ss.str());
                    return;
                }
                sections.back()->tag.sectionText.reset(new StringType(input.substr(sectionStarts.back(), tagLocationStart - sectionStarts.back())));
                sections.pop_back();
                sectionStarts.pop_back();
            }
        }
        
        // Check for sections without an ending tag
        walk([this](Component& comp, int) -> WalkControl {
            if (!comp.tag.isSectionBegin()) {
                return WalkControl::Continue;
            }
            if (comp.children.empty() || !comp.children.back().tag.isSectionEnd() || comp.children.back().tag.name != comp.tag.name) {
                streamstring ss;
                ss << "Unclosed section \"" << comp.tag.name << "\" at " << comp.position;
                errorMessage_.assign(ss.str());
                return WalkControl::Stop;
            }
            comp.children.pop_back(); // remove now useless end section component
            return WalkControl::Continue;
        });
        if (!errorMessage_.empty()) {
            return;
        }
    }
    
    enum class WalkControl {
        Continue,
        Stop,
        Skip,
    };
    using WalkCallback = std::function<WalkControl(Component&, int)>;
    
    void walk(const WalkCallback& callback) const {
        walkChildren(callback, rootComponent_);
    }

    void walkChildren(const WalkCallback& callback, const Component& comp) const {
        for (auto childComp : comp.children) {
            if (walkComponent(callback, childComp) != WalkControl::Continue) {
                break;
            }
        }
    }
    
    WalkControl walkComponent(const WalkCallback& callback, Component& comp, int depth = 0) const {
        WalkControl control{callback(comp, depth)};
        if (control == WalkControl::Stop) {
            return control;
        } else if (control == WalkControl::Skip) {
            return WalkControl::Continue;
        }
        ++depth;
        for (auto childComp : comp.children) {
            control = walkComponent(callback, childComp, depth);
            if (control == WalkControl::Stop) {
                return control;
            } else if (control == WalkControl::Skip) {
                control = WalkControl::Continue;
                break;
            }
        }
        --depth;
        return control;
    }
    
    bool isSetDelimiterValid(const StringType& delimiter) {
        // "Custom delimiters may not contain whitespace or the equals sign."
        for (const auto ch : delimiter) {
            if (ch == '=' || isspace(ch)) {
                return false;
            }
        }
        return true;
    }
    
    bool parseSetDelimiterTag(const StringType& contents, DelimiterSet& delimiterSet) {
        // Smallest legal tag is "=X X="
        if (contents.size() < 5) {
            return false;
        }
        if (contents.back() != '=') {
            return false;
        }
        const auto contentsSubstr = trim(contents.substr(1, contents.size() - 2));
        const auto spacepos = contentsSubstr.find(' ');
        if (spacepos == StringType::npos) {
            return false;
        }
        const auto nonspace = contentsSubstr.find_first_not_of(' ', spacepos + 1);
        assert(nonspace != StringType::npos);
        const StringType begin = contentsSubstr.substr(0, spacepos);
        const StringType end = contentsSubstr.substr(nonspace, contentsSubstr.size() - nonspace);
        if (!isSetDelimiterValid(begin) || !isSetDelimiterValid(end)) {
            return false;
        }
        delimiterSet.begin = begin;
        delimiterSet.end = end;
        return true;
    }
    
    void parseTagContents(bool isUnescapedVar, const StringType& contents, Tag& tag) {
        if (isUnescapedVar) {
            tag.type = Tag::Type::UnescapedVariable;
            tag.name = contents;
        } else if (contents.empty()) {
            tag.type = Tag::Type::Variable;
            tag.name.clear();
        } else {
            switch (contents.at(0)) {
                case '#':
                    tag.type = Tag::Type::SectionBegin;
                    break;
                case '^':
                    tag.type = Tag::Type::SectionBeginInverted;
                    break;
                case '/':
                    tag.type = Tag::Type::SectionEnd;
                    break;
                case '>':
                    tag.type = Tag::Type::Partial;
                    break;
                case '&':
                    tag.type = Tag::Type::UnescapedVariable;
                    break;
                case '!':
                    tag.type = Tag::Type::Comment;
                    break;
                default:
                    tag.type = Tag::Type::Variable;
                    break;
            }
            if (tag.type == Tag::Type::Variable) {
                tag.name = contents;
            } else {
                StringType name{contents};
                name.erase(name.begin());
                tag.name = trim(name);
            }
        }
    }
    
    void render(const RenderHandler& handler, Context& ctx) {
        walk([&handler, &ctx, this](Component& comp, int) -> WalkControl {
            return renderComponent(handler, ctx, comp);
        });
    }
    
    StringType render(Context& ctx) {
        std::basic_ostringstream<typename StringType::value_type> ss;
        render([&ss](const StringType& str) {
            ss << str;
        }, ctx);
        return ss.str();
    }
    
    WalkControl renderComponent(const RenderHandler& handler, Context& ctx, Component& comp) {
        if (comp.isText()) {
            handler(comp.text);
            return WalkControl::Continue;
        }
        
        const Tag& tag{comp.tag};
        const Data* var = nullptr;
        switch (tag.type) {
            case Tag::Type::Variable:
            case Tag::Type::UnescapedVariable:
                if ((var = ctx.get(tag.name)) != nullptr) {
                    if (!renderVariable(handler, var, ctx, tag.type == Tag::Type::Variable)) {
                        return WalkControl::Stop;
                    }
                }
                break;
            case Tag::Type::SectionBegin:
                if ((var = ctx.get(tag.name)) != nullptr) {
                    if (var->isLambda()) {
                        if (!renderLambda(handler, var, ctx, false, *comp.tag.sectionText, true)) {
                            return WalkControl::Stop;
                        }
                    } else if (!var->isFalse() && !var->isEmptyList()) {
                        renderSection(handler, ctx, comp, var);
                    }
                }
                return WalkControl::Skip;
            case Tag::Type::SectionBeginInverted:
                if ((var = ctx.get(tag.name)) == nullptr || var->isFalse() || var->isEmptyList()) {
                    renderSection(handler, ctx, comp, var);
                }
                return WalkControl::Skip;
            case Tag::Type::Partial:
                if ((var = ctx.get_partial(tag.name)) != nullptr && var->isPartial()) {
                    const auto partial = var->partial();
                    BasicMustache tmpl{partial()};
                    if (!tmpl.isValid()) {
                        errorMessage_ = tmpl.errorMessage();
                    } else {
                        tmpl.render(handler, ctx);
                        if (!tmpl.isValid()) {
                            errorMessage_ = tmpl.errorMessage();
                        }
                    }
                    if (!tmpl.isValid()) {
                        return WalkControl::Stop;
                    }
                }
                break;
            case Tag::Type::SetDelimiter:
                ctx.delimiterSet = *comp.tag.delimiterSet;
                break;
            default:
                break;
        }
        
        return WalkControl::Continue;
    }
    
    bool renderLambda(const RenderHandler& handler, const Data* var, Context& ctx, bool escaped, const StringType& text, bool parseWithSameContext) {
        const auto lambdaResult = var->callLambda(text);
        assert(lambdaResult.isString());
        BasicMustache tmpl = parseWithSameContext ? BasicMustache{lambdaResult.stringValue(), ctx} : BasicMustache{lambdaResult.stringValue()};
        if (!tmpl.isValid()) {
            errorMessage_ = tmpl.errorMessage();
        } else {
            const StringType str{tmpl.render(ctx)};
            if (!tmpl.isValid()) {
                errorMessage_ = tmpl.errorMessage();
            } else {
                handler(escaped ? escape(str) : str);
            }
        }
        return tmpl.isValid();
    }
    
    bool renderVariable(const RenderHandler& handler, const Data* var, Context& ctx, bool escaped) {
        if (var->isString()) {
            const auto varstr = var->stringValue();
            handler(escaped ? escape(varstr) : varstr);
        } else if (var->isLambda()) {
            return renderLambda(handler, var, ctx, escaped, {}, false);
        }
        return true;
    }

    void renderSection(const RenderHandler& handler, Context& ctx, Component& incomp, const Data* var) {
        const auto callback = [&handler, &ctx, this](Component& comp, int) -> WalkControl {
            return renderComponent(handler, ctx, comp);
        };
        if (var && var->isNonEmptyList()) {
            for (const auto& item : var->list()) {
                const ContextPusher ctxpusher{ctx, &item};
                walkChildren(callback, incomp);
            }
        } else if (var) {
            const ContextPusher ctxpusher{ctx, var};
            walkChildren(callback, incomp);
        } else {
            walkChildren(callback, incomp);
        }
    }

private:
    StringType errorMessage_;
    Component rootComponent_;
};

} // kainjow

#endif // KAINJOW_MUSTACHE_HPP
