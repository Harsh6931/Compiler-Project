#include "interpreter/environment.hpp"

#include <stdexcept>

Environment::Environment(std::shared_ptr<Environment> enclosing)
    : enclosing(std::move(enclosing)) {}

void Environment::define(const std::string& name, const Value& value) {
    values[name] = value;
}

void Environment::assign(const std::string& name, const Value& value) {
    auto it = values.find(name);
    if (it != values.end()) {
        it->second = value;
        return;
    }
    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }
    throw std::runtime_error("Undefined variable '" + name + "'");
}

Value Environment::get(const std::string& name) const {
    auto it = values.find(name);
    if (it != values.end()) {
        return it->second;
    }
    if (enclosing) {
        return enclosing->get(name);
    }
    throw std::runtime_error("Undefined variable '" + name + "'");
}
