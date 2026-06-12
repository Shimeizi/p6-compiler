#pragma once

#include <cdk/ast/basic_node.h>

namespace p6 {

  /**
   * Class for describing stop nodes.
   */
  class stop_node : public cdk::basic_node {
    size_t _level;

  public:
    stop_node(int lineno, size_t level = 1) : cdk::basic_node(lineno), _level(level) {}

    size_t level() const { return _level; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_stop_node(this, level); }
  };

} // p6

