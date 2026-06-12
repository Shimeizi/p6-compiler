#pragma once

#include <string>
#include <memory>
#include <cdk/types/basic_type.h>

namespace p6 {

  class symbol {
    std::shared_ptr<cdk::basic_type> _type;
    std::string _name;
    int _qualifier = 0; // tPUBLIC, tFOWARD, tEXTERN or 0 (private)
    int _offset = 0; // 0 (zero) means global variable/function
    bool _function = false;
    bool _defined = false;

  public:
    symbol(std::shared_ptr<cdk::basic_type> type, const std::string &name, int qualifier = 0,
          bool function = false, bool defined = true) :
        _type(type), _name(name), _qualifier(qualifier), _function(function), _defined(defined) {
    }

    virtual ~symbol() {
      // EMPTY
    }

    std::shared_ptr<cdk::basic_type> type() const {
      return _type;
    }

    void set_type(std::shared_ptr<cdk::basic_type> type) {
      _type = type;
    }

    bool is_typed(cdk::typename_type name) const {
      return _type && _type->name() == name;
    }

    const std::string &name() const {
      return _name;
    }

    int qualifier() const {
      return _qualifier;
    }

    void set_qualifier(int qualifier) {
      _qualifier = qualifier;
    }

    int offset() const {
      return _offset;
    }
    
    int set_offset(int offset) {
      return _offset = offset;
    }

    // TO DELETE: use offset() instead
    int value() const {
      return _offset;
    }

    // TO DELETE: use offset(int) instead
    int value(int value) {
      return _offset = value;
    }

    bool is_function() const {
      return _function;
    }

    void set_function(bool function) {
      _function = function;
    }

    bool defined() const {
      return _defined;
    }

    void set_defined(bool defined) {
      _defined = defined;
    }
  };

  inline auto make_symbol(int qualifier, std::shared_ptr<cdk::basic_type> type, const std::string &name,
                          bool function = false, bool defined = true) {
    return std::make_shared<p6::symbol>(type, name, qualifier, function, defined);
  }

} // p6

