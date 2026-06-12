#pragma once

#include <cdk/ast/expression_node.h>

namespace p6 {

  /**
   * Class for describing null nodes.
   */
  class null_node : public cdk::expression_node {
    
  public:
    null_node(int lineno) : cdk::expression_node(lineno) {}

    void accept(basic_ast_visitor *sp, int level) override { sp->do_null_node(this, level); }
  };

} // p6

