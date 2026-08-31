#ifndef VALUE_HPP
#define VALUE_HPP

#include "ast/ast.hpp"

#include <memory>
#include <string>
#include <vector>

class Environment;

enum class InterpNativeId { Len };

// Runtime value used by the tree-walking interpreter.
struct Value {
    enum class Type { Nil, Number, Boolean, String, Function, Array, Native };

    Type type = Type::Nil;
    long long number = 0;
    bool boolean = false;
    std::string string;
    const FunctionDeclStmt* declaration = nullptr;
    std::shared_ptr<Environment> closure;
    std::shared_ptr<std::vector<Value>> array;
    InterpNativeId nativeId = InterpNativeId::Len;

    static Value makeNil() { return Value{}; }

    static Value makeNumber(long long n) {
        Value v;
        v.type = Type::Number;
        v.number = n;
        return v;
    }

    static Value makeBoolean(bool b) {
        Value v;
        v.type = Type::Boolean;
        v.boolean = b;
        return v;
    }

    static Value makeString(std::string s) {
        Value v;
        v.type = Type::String;
        v.string = std::move(s);
        return v;
    }

    static Value makeFunction(const FunctionDeclStmt* decl,
                              std::shared_ptr<Environment> env) {
        Value v;
        v.type = Type::Function;
        v.declaration = decl;
        v.closure = std::move(env);
        return v;
    }

    static Value makeArray(std::shared_ptr<std::vector<Value>> elements) {
        Value v;
        v.type = Type::Array;
        v.array = std::move(elements);
        return v;
    }

    static Value makeNative(InterpNativeId id) {
        Value v;
        v.type = Type::Native;
        v.nativeId = id;
        return v;
    }

    bool isTruthy() const {
        if (type == Type::Nil) {
            return false;
        }
        if (type == Type::Boolean) {
            return boolean;
        }
        return true;
    }

    std::string toString() const {
        switch (type) {
            case Type::Nil:
                return "nil";
            case Type::Number:
                return std::to_string(number);
            case Type::Boolean:
                return boolean ? "true" : "false";
            case Type::String:
                return string;
            case Type::Function:
                return "<fn " + declaration->name + ">";
            case Type::Array: {
                std::string out = "[";
                for (size_t i = 0; i < array->size(); ++i) {
                    if (i > 0) out += ", ";
                    out += (*array)[i].toString();
                }
                out += "]";
                return out;
            }
            case Type::Native:
                return "<native fn>";
        }
        return "nil";
    }
};

#endif
