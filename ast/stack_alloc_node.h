#pragma once

#include <cdk/ast/unary_operation_node.h>

namespace p6 {

  /**
   * Class for describing stack allocation nodes.
   */
  class stack_alloc_node : public cdk::unary_operation_node {

  public:
    stack_alloc_node(int lineno, cdk::expression_node *argument) :
        cdk::unary_operation_node(lineno, argument) {
    }

    void accept(basic_ast_visitor *sp, int level) override { sp->do_stack_alloc_node(this, level); }
  };

} // p6

