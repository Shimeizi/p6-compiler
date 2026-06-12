#pragma once

#include "targets/basic_ast_visitor.h"

namespace p6 {

  /**
   * Check types of AST nodes.
   */
  class type_checker: public basic_ast_visitor {
    cdk::symbol_table<p6::symbol> &_symtab;
    basic_ast_visitor *_parent;
    std::shared_ptr<p6::symbol> _current_function = nullptr;
    int _in_loop = 0;

  public:
    type_checker(std::shared_ptr<cdk::compiler> compiler, cdk::symbol_table<p6::symbol> &symtab, basic_ast_visitor *parent) :
        basic_ast_visitor(compiler), _symtab(symtab), _parent(parent) {
    }

  public:
    ~type_checker() {
      os().flush();
    }

  protected:
    void processUnaryExpression(cdk::unary_operation_node *const node, int lvl);
    void processBinaryExpression(cdk::binary_operation_node *const node, int lvl);
    template<typename T>
    void process_literal(cdk::literal_node<T> *const node, int lvl) {
    }

  protected:
    void do_IDPExpression(cdk::binary_operation_node *const node, int lvl);

  protected:
    void do_ScalarLogicalExpression(cdk::binary_operation_node * const node, int lvl);
    void do_BooleanLogicalExpression(cdk::binary_operation_node * const node, int lvl);
    void do_GeneralLogicalExpression(cdk::binary_operation_node * const node, int lvl);

  public:
    // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
    // do not edit these lines: end

  };

} // p6

//---------------------------------------------------------------------------
//     HELPER MACRO FOR TYPE CHECKING
//---------------------------------------------------------------------------

#define CHECK_TYPES(compiler, symtab, node) { \
  try { \
    p6::type_checker checker(compiler, symtab, this); \
    (node)->accept(&checker, 0); \
  } \
  catch (const std::string &problem) { \
    std::cerr << (node)->lineno() << ": " << problem << std::endl; \
    return; \
  } \
}

#define ASSERT_SAFE_EXPRESSIONS CHECK_TYPES(_compiler, _symtab, node)

