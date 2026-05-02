#ifndef MORSE_AST_H
#define MORSE_AST_H

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

enum class DataType {
    Int,
    Float,
    Invalid
};

inline const char* data_type_name(DataType type) {
    switch (type) {
        case DataType::Int:
            return "int";
        case DataType::Float:
            return "float";
        case DataType::Invalid:
        default:
            return "invalid";
    }
}

struct SemanticError {
    std::string message;
    int line;
    int column;
};

struct Symbol {
    std::string name;
    DataType type;
};

class SymbolTable {
public:
    void enter_scope() {
        scopes_.emplace_back();
    }

    void leave_scope() {
        if (!scopes_.empty()) {
            scopes_.pop_back();
        }
    }

    bool define(const std::string& name, DataType type) {
        if (scopes_.empty()) {
            enter_scope();
        }

        auto& current = scopes_.back();
        if (current.find(name) != current.end()) {
            return false;
        }

        current.emplace(name, Symbol{name, type});
        return true;
    }

    const Symbol* lookup(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }

        return nullptr;
    }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};

class Node {
public:
    Node(std::string kind, std::string lexeme = "", DataType type = DataType::Invalid)
        : kind_(std::move(kind)), lexeme_(std::move(lexeme)), type_(type), line_(0), column_(0) {}

    void set_location(int line, int column) {
        line_ = line;
        column_ = column;
    }

    int line() const { return line_; }
    int column() const { return column_; }

    void add_child(Node* child) {
        if (child != nullptr) {
            children_.emplace_back(child);
        }
    }

    void set_type(DataType type) {
        type_ = type;
    }

    DataType type() const {
        return type_;
    }

    const std::string& kind() const { return kind_; }
    const std::string& lexeme() const { return lexeme_; }
    size_t num_children() const { return children_.size(); }
    Node* child(size_t index) const { return index < children_.size() ? children_[index].get() : nullptr; }
    
    Node* release_child(size_t index) {
        if (index < children_.size()) {
            return children_[index].release();
        }
        return nullptr;
    }

    void replace_child(size_t index, Node* new_child) {
        if (index < children_.size() && new_child != nullptr) {
            children_[index].reset(new_child);
        }
    }

    void print(std::ostream& out, int indent = 0) const {
        out << std::string(indent, ' ') << kind_;
        if (!lexeme_.empty()) {
            out << " [" << lexeme_ << "]";
        }
        out << " <" << data_type_name(type_) << ">\n";
        for (const auto& child : children_) {
            child->print(out, indent + 2);
        }
    }

private:
    std::string kind_;
    std::string lexeme_;
    DataType type_;
    int line_;
    int column_;
    std::vector<std::unique_ptr<Node>> children_;
};

#endif
