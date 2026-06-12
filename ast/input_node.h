#pragma once

#include <cdk/ast/expression_node.h>

namespace p6 {

  /**
   * Class for describing read nodes.
   */
  class input_node : public cdk::expression_node {

  public:
    input_node(int lineno) : cdk::expression_node(lineno) {}

    void accept(basic_ast_visitor *sp, int level) { sp->do_input_node(this, level); }

  };

} // p6

