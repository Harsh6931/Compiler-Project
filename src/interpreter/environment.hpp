#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include "interpreter/value.hpp"

#include <memory>
#include <string>
#include <unordered_map>

// Variable scope: name -> Value, with optional parent (enclosing) scope.
class Environment {
public:
    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> enclosing);

    void define(const std::string& name, const Value& value);
    void assign(const std::string& name, const Value& value);
    Value get(const std::string& name) const;

    std::shared_ptr<Environment> enclosing;

private:
    std::unordered_map<std::string, Value> values;
};

#endif
