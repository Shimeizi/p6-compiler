#pragma once

#include <string>
#include <cdk/ast/typed_node.h>

namespace p6 {

  /**
   * Class for describing function declarations nodes.
   * 
   * <pre>
   * declaration: qualifier funcType identifier ';'
   *            {
   *              // functional type (funcType) examples: int<int>, void<>, [void]<int>
   *              new p6::function_declaration_node(LINE, qualifier, funcType, identifier);
   *            }
   * </pre>
   */
  class function_declaration_node: public cdk::typed_node {
    int _qualifier;
    std::string _identifier;

  public:
    function_declaration_node(int lineno, int qualifier, std::shared_ptr<cdk::basic_type> funcType,
                              const std::string &identifier) :
        cdk::typed_node(lineno), _qualifier(qualifier), _identifier(identifier) {
      type(funcType);
    }

    int qualifier() { return _qualifier; }

    const std::string& identifier() const { return _identifier; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_function_declaration_node(this, level); }
  
  };

} // p6

