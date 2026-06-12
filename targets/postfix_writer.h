#pragma once

#include "targets/basic_ast_visitor.h"

#include <set>
#include <stack>
#include <sstream>
#include <vector>
#include <cdk/ast/binary_operation_node.h>
#include <cdk/emitters/basic_postfix_emitter.h>

namespace p6 {

  //!
  //! Traverse syntax tree and generate the corresponding assembly code.
  //!
  class postfix_writer: public basic_ast_visitor {
    cdk::symbol_table<p6::symbol> &_symtab;

    std::set<std::string> _externals_to_declare;
    
    // semantic analysis
    bool _errors, _inFunction, _inFunctionName, _inFunctionArgs, _inFunctionBody;
    bool _returnSeen; // when building a function
    
    std::vector<int> _whileIni;
    std::vector<int> _whileEnd;

    std::stack<bool> _globals; // for deciding whether a variable is global or not
    std::shared_ptr<p6::symbol> _function; // for keeping track of the current function and its arguments
    int _offset; // current framepointer offset (0 means no vars defined)

    // code generation
    cdk::basic_postfix_emitter &_pf;
    int _lbl;

  public:
    postfix_writer(std::shared_ptr<cdk::compiler> compiler, cdk::symbol_table<p6::symbol> &symtab, cdk::basic_postfix_emitter &pf) :
        basic_ast_visitor(compiler), _symtab(symtab), _errors(false), _inFunction(false), _inFunctionName(false),
        _inFunctionArgs(false), _inFunctionBody(false), _returnSeen(false), _function(nullptr), _offset(0),
        _pf(pf), _lbl(0) {
      _globals.push(true);
    }

  public:
    ~postfix_writer() {
      os().flush();
    }

    
    private:
    /** Method used to generate sequential labels. */
    inline std::string mklbl(int lbl) {
      std::ostringstream oss;
      if (lbl < 0)
      oss << ".L" << -lbl;
      else
      oss << "_L" << lbl;
      return oss.str();
    }
    
    void process_comparison(cdk::binary_operation_node *const node, int lvl, const std::string &op);

  public:
  // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end

  };

} // p6

