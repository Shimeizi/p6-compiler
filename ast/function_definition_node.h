#pragma once

#include <string>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/typed_node.h>
#include "ast/block_node.h"

namespace p6 {

  /**
   * Class for describing function definition nodes.
   *
   * <pre>
   * function_definition:
   *   opt_qualifier identifier '(' opt_args ')' '->' data_type block
   *
   * Example:
   *   factorial(int n) -> int {
   *     ...
   *   }
   * </pre>
   */
  class function_definition_node : public cdk::typed_node {
    int _qualifier;
    std::string _identifier;
    cdk::sequence_node *_arguments;
    p6::block_node *_block;

  public:
    function_definition_node(int lineno, int qualifier, std::shared_ptr<cdk::basic_type> return_type, const std::string &identifier,
                             cdk::sequence_node *arguments, p6::block_node *block) :
        cdk::typed_node(lineno), _qualifier(qualifier), _identifier(identifier), _arguments(arguments), _block(block) {
     
      std::vector<std::shared_ptr<cdk::basic_type>> input_types;

      for (size_t i = 0; i < _arguments->size(); i++) {
        auto arg = dynamic_cast<cdk::typed_node *>(_arguments->node(i));
        input_types.push_back(arg->type());
      }

      type(cdk::functional_type::create(input_types, return_type));

    }

    int qualifier() { return _qualifier; }
    
    const std::string& identifier() const { return _identifier; }

    cdk::sequence_node* arguments() { return _arguments; }

    cdk::typed_node* argument(size_t ax) { return dynamic_cast<cdk::typed_node*>(_arguments->node(ax)); }

    p6::block_node* block() { return _block; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_function_definition_node(this, level);}
  
  };

} // p6

