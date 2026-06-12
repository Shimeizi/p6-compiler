#pragma once

#include <cdk/ast/expression_node.h>

namespace p6 {

  /**
   * Class for describing conditional-iterate nodes.
   */
  class conditional_iterate_node : public cdk::basic_node {
    cdk::expression_node *_condition;
    cdk::expression_node *_vector;
    cdk::expression_node *_count;
    const std::string _identifier;

public:
    conditional_iterate_node(int lineno, cdk::expression_node *condition, cdk::expression_node *vector, cdk::expression_node *count, const std::string &identifier,) :
        basic_node(lineno), _condition(condition), _vector(vector), _count(count), _identifier(identifier) {
    }

    cdk::expression_node *condition() { return _condition; }

    cdk::expression_node *vector() { return _vector; }

    cdk::expression_node *count() { return _count; }

    const std::string& identifier() const { return _identifier; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_conditional_iterate_node(this, level); }

  };

} // p6


