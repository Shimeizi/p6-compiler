#include <string>
#include <sstream>
#include "targets/type_checker.h"
#include "targets/postfix_writer.h"
#include "targets/frame_size_calculator.h"
#include "targets/symbol.h"
#include ".auto/all_nodes.h"  // all_nodes.h is automatically generated
// must come after other #includes
#include "p6_parser.tab.h"

//---------------------------------------------------------------------------

void p6::postfix_writer::do_nil_node(cdk::nil_node * const node, int lvl) {
  // EMPTY
}
void p6::postfix_writer::do_data_node(cdk::data_node * const node, int lvl) {
  // EMPTY
}
void p6::postfix_writer::do_double_node(cdk::double_node * const node, int lvl) {
  // EMPTY
}
void p6::postfix_writer::do_balanced3_node(cdk::balanced3_node * const node, int lvl) {
  if (_inFunctionBody) {
    _pf.BALANCED3(node->value());
  } else {
    _pf.SBALANCED3(node->value());
  }
}
void p6::postfix_writer::do_posit3_node(cdk::posit3_node * const node, int lvl) {
  // EMPTY
}
void p6::postfix_writer::do_takum3_node(cdk::takum3_node * const node, int lvl) {
  if (_inFunctionBody) {
    _pf.TAKUM3(node->value());
  } else {
    _pf.STAKUM3(node->value());
  }
}
void p6::postfix_writer::do_not_node(cdk::not_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  _pf.KNOT();
}
void p6::postfix_writer::do_and_node(cdk::and_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP64();
  _pf.B2I();
  _pf.INT(0);
  // short-circuit if left is false (0)
  _pf.JLT(mklbl(lbl));
  node->right()->accept(this, lvl + 2);
  _pf.KAND();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}
void p6::postfix_writer::do_or_node(cdk::or_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP64();
  _pf.B2I();
  _pf.INT(0);
  // short-circuit if left is true
  _pf.JGT(mklbl(lbl));
  node->right()->accept(this, lvl + 2);
  _pf.KOR();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_sequence_node(cdk::sequence_node * const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl + 2);
  }
}

void p6::postfix_writer::do_block_node(p6::block_node * const node, int lvl) {
  _symtab.push(); // block-local scope
  if (node->declarations()) node->declarations()->accept(this, lvl + 2);
  if (node->instructions()) node->instructions()->accept(this, lvl + 2);
  _symtab.pop();
}

void p6::postfix_writer::do_return_node(p6::return_node * const node, int lvl) {
  _returnSeen = true;
  auto return_type = cdk::functional_type::cast(_function->type())->output(0);

  if (return_type->name() != cdk::TYPE_VOID) {
    if (node->expression()) {
      node->expression()->accept(this, lvl + 2);
    }

    if (_function->name().empty()) {
      if (node->expression() && node->expression()->is_typed(cdk::TYPE_BALANCED3)) {
        _pf.B2I();
      }
      _pf.STFVAL32I();

    } else if (return_type->name() == cdk::TYPE_BALANCED3) {
      _pf.STFVAL64I();

    } else if (return_type->name() == cdk::TYPE_STRING || return_type->name() == cdk::TYPE_POINTER) {
      _pf.STFVAL32I();

    } else if (return_type->name() == cdk::TYPE_TAKUM3) {
      if (node->expression() && node->expression()->is_typed(cdk::TYPE_BALANCED3)) {
        _pf.B2T();
      }

      // takum3 is returned through a hidden pointer argument
      _pf.LOCAL(8);
      _pf.LDINT();
      _pf.STTAKUM3();
    }
  }

  _pf.LEAVE();
  _pf.RET();
}

void p6::postfix_writer::do_variable_declaration_node(p6::variable_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  auto id = node->identifier();
  int offset = 0;
  int typesize = node->type()->size();

  if (_inFunctionBody) { // local variables: negative offsets
    _offset -= typesize;
    offset = _offset;
  } else if (_inFunctionArgs) { // function arguments: positive offsets
    offset = _offset;
    _offset += typesize;
  } else { // global variables
    offset = 0;
  }

  auto symbol = new_symbol();
  if (symbol) {
    symbol->set_offset(offset);
    reset_new_symbol();
  }

  // arguments: only set offset, no code generation
  if (_inFunctionArgs) {
    return;
  }

  // local variables
  if (_inFunctionBody) {
    if (node->initializer()) {
      node->initializer()->accept(this, lvl + 2);

      if (node->is_typed(cdk::TYPE_TAKUM3) && node->initializer()->is_typed(cdk::TYPE_BALANCED3)) {
        _pf.B2T();
      }
      _pf.LOCAL(offset);

      if (node->is_typed(cdk::TYPE_BALANCED3)) {
        _pf.STBALANCED3();
      } else if (node->is_typed(cdk::TYPE_TAKUM3)) {
        _pf.STTAKUM3();
      } else {
        // string, pointer values: 32-bit addresses/values
        _pf.STINT();
      }
    }
    return;
  }

  // forward/external declarations do not allocate storage
  if (node->qualifier() == tFORWARD || node->qualifier() == tEXTERN) {
    _externals_to_declare.insert(id);
    return;
  }

  // global variables without initializer
  if (node->initializer() == nullptr) {
    _pf.BSS();
    _pf.ALIGN();
    if (node->qualifier() == tPUBLIC) {
      _pf.GLOBAL(id, _pf.OBJ());
    }
    _pf.LABEL(id);
    _pf.SALLOC(typesize);
    return;
  }

  // global variables with initializer
  _pf.DATA();
  _pf.ALIGN();
  if (node->qualifier() == tPUBLIC) {
    _pf.GLOBAL(id, _pf.OBJ());
  }
  _pf.LABEL(id);

  if (node->is_typed(cdk::TYPE_BALANCED3)) {
    node->initializer()->accept(this, lvl + 2);
  } else if (node->is_typed(cdk::TYPE_TAKUM3)) {
    // global real initializers must be emitted statically
    if (node->initializer()->is_typed(cdk::TYPE_TAKUM3)) {
      node->initializer()->accept(this, lvl + 2);
    } else {
      auto binit = dynamic_cast<cdk::balanced3_node *>(node->initializer());
      if (binit == nullptr) {
        throw std::string("internal error: non-literal balanced3 initializer for global takum3");
      }
      // integer literals can be converted to takum3 at compile time
      cdk::takum3_type::value_type tvalue(binit->value());
      _pf.STAKUM3(tvalue);
    }
  } else if (node->is_typed(cdk::TYPE_STRING) || node->is_typed(cdk::TYPE_POINTER)) {
    node->initializer()->accept(this, lvl + 2);
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_integer_node(cdk::integer_node * const node, int lvl) {
  // EMPTY
}

void p6::postfix_writer::do_string_node(cdk::string_node * const node, int lvl) {
  int lbl1;

  // store string contents; the string value is its label address.
  _pf.RODATA(); 
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1 = ++_lbl)); // create a label for the string.
  _pf.SSTRING(node->value());
  
  if (_inFunctionBody) {
    _pf.TEXT();
    _pf.ADDR(mklbl(lbl1));
  } else {
    _pf.DATA();
    _pf.SADDR(mklbl(lbl1));
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_unary_minus_node(cdk::unary_minus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl + 2);

  if (node->argument()->is_typed(cdk::TYPE_BALANCED3)) {
    _pf.BNEG();
    return;
  } else if (node->argument()->is_typed(cdk::TYPE_TAKUM3)) {
    _pf.TNEG();
    return;
  } else {
    // should not happen
  }
}

void p6::postfix_writer::do_unary_plus_node(cdk::unary_plus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl + 2);
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_add_node(cdk::add_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (node->is_typed(cdk::TYPE_TAKUM3)) {
    node->left()->accept(this, lvl + 2);
    if (node->left()->is_typed(cdk::TYPE_BALANCED3)) {
      _pf.B2T();
    }
    node->right()->accept(this, lvl + 2);
    if (node->right()->is_typed(cdk::TYPE_BALANCED3)) {
      _pf.B2T();
    }
    _pf.TADD();
    return;
  } else if (node->is_typed(cdk::TYPE_BALANCED3)) {
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _pf.BADD();
    return;
  } else if (node->is_typed(cdk::TYPE_POINTER)) {
    // compute address: pointer + index * sizeof(element)
    if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_BALANCED3)) {
      auto ref = cdk::reference_type::cast(node->left()->type());
      int element_size = ref->referenced()->size();

      node->left()->accept(this, lvl + 2);
      node->right()->accept(this, lvl + 2);
      _pf.B2I();
      _pf.INT(element_size);
      _pf.MUL();
      _pf.ADD();
      return;
    } else if (node->left()->is_typed(cdk::TYPE_BALANCED3) && node->right()->is_typed(cdk::TYPE_POINTER)) {

      auto ref = cdk::reference_type::cast(node->right()->type());
      int element_size = ref->referenced()->size();

      node->left()->accept(this, lvl + 2);
      _pf.B2I();
      _pf.INT(element_size);
      _pf.MUL();
      node->right()->accept(this, lvl + 2);
      _pf.ADD();
      return;
    } else {
      // should not happen
    }
  }
}

void p6::postfix_writer::do_sub_node(cdk::sub_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (node->is_typed(cdk::TYPE_TAKUM3)) {
    node->left()->accept(this, lvl + 2);
    if (node->left()->is_typed(cdk::TYPE_BALANCED3)) {
      _pf.B2T();
    }
    node->right()->accept(this, lvl + 2);
    if (node->right()->is_typed(cdk::TYPE_BALANCED3)) {
      _pf.B2T();
    }
    _pf.TSUB();
    return;
  } else if (node->is_typed(cdk::TYPE_BALANCED3)) {
    if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)) {

      auto ref = cdk::reference_type::cast(node->left()->type());
      int element_size = ref->referenced()->size();
      // compute distance: (address1 - address2) / sizeof(element)
      node->left()->accept(this, lvl + 2);
      node->right()->accept(this, lvl + 2);
      _pf.SUB();
      _pf.INT(element_size);
      _pf.DIV();
      _pf.I2B();
      return;
    } else {
      node->left()->accept(this, lvl + 2);
      node->right()->accept(this, lvl + 2);
      _pf.BSUB();
      return;
    }
  } else if (node->is_typed(cdk::TYPE_POINTER)) {
    if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_BALANCED3)) {
      auto ref = cdk::reference_type::cast(node->left()->type());
      int element_size = ref->referenced()->size();
      // compute address: pointer - index * sizeof(element)
      node->left()->accept(this, lvl + 2);
      node->right()->accept(this, lvl + 2);
      _pf.B2I();
      _pf.INT(element_size);
      _pf.MUL();
      _pf.SUB();
      return;
    } else {
      // should not happen
    }
  }
}

void p6::postfix_writer::do_mul_node(cdk::mul_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (node->is_typed(cdk::TYPE_TAKUM3)) {
    node->left()->accept(this, lvl + 2);
    if (node->left()->is_typed(cdk::TYPE_BALANCED3)) _pf.B2T();
    node->right()->accept(this, lvl + 2);
    if (node->right()->is_typed(cdk::TYPE_BALANCED3)) _pf.B2T();
    _pf.TMUL();
    return;
  } else if (node->is_typed(cdk::TYPE_BALANCED3)) {
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _pf.BMUL();
    return;
  } else {
    // should not happen
  }
}
void p6::postfix_writer::do_div_node(cdk::div_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (node->is_typed(cdk::TYPE_TAKUM3)) {
    node->left()->accept(this, lvl + 2);
    if (node->left()->is_typed(cdk::TYPE_BALANCED3)) _pf.B2T();
    node->right()->accept(this, lvl + 2);
    if (node->right()->is_typed(cdk::TYPE_BALANCED3)) _pf.B2T();
    _pf.TDIV();
    return;
  } else if (node->is_typed(cdk::TYPE_BALANCED3)) {
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _pf.BDIV();
    return;
  } else {
    // should not happen
  }
}
void p6::postfix_writer::do_mod_node(cdk::mod_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->is_typed(cdk::TYPE_BALANCED3)) {
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _pf.BMOD();
    return;
  } else {
    // should not happen
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_sizeof_node(p6::sizeof_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  cdk::balanced3_type::value_type value(static_cast<int>(node->expression()->type()->size()));
  if (_inFunctionBody) {
    _pf.BALANCED3(value);
  } else {
    _pf.SBALANCED3(value);
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::process_comparison(cdk::binary_operation_node *const node, int lvl, const std::string &op) {
  int lbl_false, lbl_end;

  bool takum_cmp = node->left()->is_typed(cdk::TYPE_TAKUM3) || node->right()->is_typed(cdk::TYPE_TAKUM3);
  bool pointer_cmp = node->left()->is_typed(cdk::TYPE_POINTER) || node->right()->is_typed(cdk::TYPE_POINTER);

  if (takum_cmp) {
    node->left()->accept(this, lvl + 2);
    if (node->left()->is_typed(cdk::TYPE_BALANCED3)) {
      _pf.B2T();
    }
    node->right()->accept(this, lvl + 2);
    if (node->right()->is_typed(cdk::TYPE_BALANCED3)) {
      _pf.B2T();
    }
    // compare the takum3 difference to preserve precision
    _pf.TSUB();
    _pf.T2D();
    _pf.DOUBLE(0.0);
    _pf.DCMP();
    _pf.INT(0);
  } else if (pointer_cmp) {
    // pointer values are already 32-bit addresses
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
  } else {
    // balanced3 values are converted to binary integers for comparison
    node->left()->accept(this, lvl + 2);
    _pf.B2I();
    node->right()->accept(this, lvl + 2);
    _pf.B2I();
  }

  if (op == "lt") {
    _pf.LT();
  } else if (op == "le") {
    _pf.LE();
  } else if (op == "gt") {
    _pf.GT();
  } else if (op == "ge") {
    _pf.GE();
  } else if (op == "eq") {
    _pf.EQ();
  } else if (op == "ne") {
    _pf.NE();
  } else {
    // should not happen
  }

  _pf.JZ(mklbl(lbl_false = ++_lbl));
  _pf.BALANCED3(cdk::balanced3_type::value_type(1));
  _pf.JMP(mklbl(lbl_end = ++_lbl));

  _pf.LABEL(mklbl(lbl_false));
  _pf.BALANCED3(cdk::balanced3_type::value_type(-1));

  _pf.LABEL(mklbl(lbl_end));
}
void p6::postfix_writer::do_lt_node(cdk::lt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  process_comparison(node, lvl, "lt");
}

void p6::postfix_writer::do_le_node(cdk::le_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  process_comparison(node, lvl, "le");
}

void p6::postfix_writer::do_gt_node(cdk::gt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  process_comparison(node, lvl, "gt");
}

void p6::postfix_writer::do_ge_node(cdk::ge_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  process_comparison(node, lvl, "ge");
}

void p6::postfix_writer::do_eq_node(cdk::eq_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  process_comparison(node, lvl, "eq");
}

void p6::postfix_writer::do_ne_node(cdk::ne_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  process_comparison(node, lvl, "ne");
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_address_of_node(p6::address_of_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  // lvalue code already leaves an address
  node->lvalue()->accept(this, lvl + 2);
}

void p6::postfix_writer::do_stack_alloc_node(p6::stack_alloc_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl + 2);
  _pf.B2I();

  auto reftype = cdk::reference_type::cast(node->type());
  int element_size = reftype->referenced()->size();

  // allocate n elements and leave the base address on the stack
  _pf.INT(element_size);
  _pf.MUL();
  _pf.ALLOC();
  _pf.SP();
}

void p6::postfix_writer::do_null_node(p6::null_node * const node, int lvl) {
  // null is represented as address 0
  if (_inFunctionBody) {
    _pf.INT(0);
  } else {
    _pf.SINT(0);
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_variable_node(cdk::variable_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  const std::string &id = node->name();
  auto symbol = _symtab.find(id);

  if (symbol->offset() == 0) { // global
    _pf.ADDR(symbol->name());
  } else {
    _pf.LOCAL(symbol->offset());
  }
}

void p6::postfix_writer::do_index_node(p6::index_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->base()->accept(this, lvl + 2);
  node->index()->accept(this, lvl + 2);
  // compute address: base + index * sizeof(element)
  _pf.B2I();
  _pf.INT(node->type()->size());
  _pf.MUL();
  _pf.ADD();
}

void p6::postfix_writer::do_rvalue_node(cdk::rvalue_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl);

  // lvalue code leaves an address; load the value stored there
  if (node->is_typed(cdk::TYPE_BALANCED3)) {
    _pf.LDBALANCED3();
  } else if (node->is_typed(cdk::TYPE_TAKUM3)) {
    _pf.LDTAKUM3();
  } else if (node->is_typed(cdk::TYPE_STRING) || node->is_typed(cdk::TYPE_POINTER) || node->is_typed(cdk::TYPE_FUNCTIONAL)) {
    _pf.LDINT();
  }
}

void p6::postfix_writer::do_assignment_node(cdk::assignment_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->rvalue()->accept(this, lvl + 2);

  if (node->lvalue()->is_typed(cdk::TYPE_TAKUM3) && node->rvalue()->is_typed(cdk::TYPE_BALANCED3)) {
    _pf.B2T();
  }

  // keep assignment value on the stack after storing it
  if (node->lvalue()->is_typed(cdk::TYPE_TAKUM3)) {
    _pf.DUP128();
  } else if (node->lvalue()->is_typed(cdk::TYPE_BALANCED3)) {
    _pf.DUP64();
  } else { // string and pointer
    _pf.DUP32();
  }

  node->lvalue()->accept(this, lvl + 2);
  if (node->lvalue()->is_typed(cdk::TYPE_BALANCED3)) {
    _pf.STBALANCED3();
  } else if (node->lvalue()->is_typed(cdk::TYPE_TAKUM3)) {
    _pf.STTAKUM3();
  } else { // string and pointer
    _pf.STINT();
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_program_node(p6::program_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _function = p6::make_symbol(node->qualifier(), node->type(), "", true, true);
  reset_new_symbol();

  // generate the main function (RTS mandates that its name be "_main")
  _pf.TEXT();
  _pf.ALIGN();
  _pf.GLOBAL("_main", _pf.FUNC());
  _pf.LABEL("_main");

  _inFunction = true;
  _inFunctionBody = true;
  _returnSeen = false;
  _globals.push(false);
  _offset = 0;

  p6::frame_size_calculator lsc(_compiler, _symtab);
  node->block()->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize());

  node->block()->accept(this, lvl);

  // default return value for main when there is no explicit return
  if (!_returnSeen) {
    _pf.INT(0);
    _pf.STFVAL32I();
    _pf.LEAVE();
    _pf.RET();
  }

  _globals.pop();
  _inFunctionBody = false;
  _inFunction = false;
  _function = nullptr;

  for (const std::string &s : _externals_to_declare) {
    _pf.EXTERN(s);
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_evaluation_node(p6::evaluation_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl + 2);
  if (!node->argument()->is_typed(cdk::TYPE_VOID)) {
    _pf.TRASH(node->argument()->type()->size());
  }
}

void p6::postfix_writer::do_print_node(p6::print_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  for (size_t i = 0; i < node->arguments()->size(); i++) {
    auto argument = dynamic_cast<cdk::expression_node*>(node->arguments()->node(i));

    argument->accept(this, lvl + 2);

    if (argument->is_typed(cdk::TYPE_BALANCED3)) {
      _externals_to_declare.insert("balanced3_print");
      _pf.CALL("balanced3_print");
      _pf.TRASH(8);
    } else if (argument->is_typed(cdk::TYPE_TAKUM3)) {
      _externals_to_declare.insert("takum3_print");
      _pf.CALL("takum3_print");
      _pf.TRASH(16);
    } else if (argument->is_typed(cdk::TYPE_STRING)) {
      _externals_to_declare.insert("prints");
      _pf.CALL("prints");
      _pf.TRASH(4);
    }
  }

  if (node->newline()) {
    _externals_to_declare.insert("println");
    _pf.CALL("println");
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_input_node(p6::input_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->is_typed(cdk::TYPE_BALANCED3)) {
    _externals_to_declare.insert("balanced3_read");
    _pf.CALL("balanced3_read");
    _pf.LDFVAL64I();

  } else if (node->is_typed(cdk::TYPE_TAKUM3)) {
    _externals_to_declare.insert("takum3_read");
    _pf.INT(16);
    _pf.ALLOC();
    _pf.SP();

    _pf.CALL("takum3_read");
    _pf.TRASH(4); // remove hidden result pointer; the 16-byte result remains on stack

  } else if (node->is_typed(cdk::TYPE_STRING)) {
    _externals_to_declare.insert("readln");
    _pf.CALL("readln");
    _pf.LDFVAL32I();
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_while_node(p6::while_node * const node, int lvl) {
  int lbl_begin, lbl_end;

  _whileIni.push_back(lbl_begin = ++_lbl);
  _whileEnd.push_back(lbl_end = ++_lbl);

  _pf.LABEL(mklbl(lbl_begin));

  node->condition()->accept(this, lvl + 2);
  _pf.B2I();
  _pf.INT(0);
  // exit loop if condition is false (0 or negative)
  _pf.JLE(mklbl(lbl_end));

  node->block()->accept(this, lvl + 2);

  _pf.JMP(mklbl(lbl_begin));
  _pf.LABEL(mklbl(lbl_end));

  _whileIni.pop_back();
  _whileEnd.pop_back();
}

void p6::postfix_writer::do_next_node(p6::next_node * const node, int lvl) {
  size_t index = _whileIni.size() - node->level();
  _pf.JMP(mklbl(_whileIni[index]));
}

void p6::postfix_writer::do_stop_node(p6::stop_node * const node, int lvl) {
  size_t index = _whileEnd.size() - node->level();
  _pf.JMP(mklbl(_whileEnd[index]));
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_if_node(p6::if_node * const node, int lvl) {
  int lbl1;
  node->condition()->accept(this, lvl + 2);
  _pf.B2I();
  _pf.INT(0);
  _pf.JLE(mklbl(lbl1 = ++_lbl));
  node->block()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl1));
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_if_else_node(p6::if_else_node * const node, int lvl) {
  int lbl_else, lbl_end;
  node->condition()->accept(this, lvl + 2);
  _pf.B2I();
  _pf.INT(0);
  _pf.JLE(mklbl(lbl_else = ++_lbl));
  node->thenblock()->accept(this, lvl + 2);
  _pf.JMP(mklbl(lbl_end = ++_lbl));
  _pf.LABEL(mklbl(lbl_else));
  node->elseblock()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl_end));
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_function_call_node(p6::function_call_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  auto symbol = _symtab.find(node->identifier());
  auto ftype = cdk::functional_type::cast(symbol->type());
  auto return_type = ftype->output(0);
  size_t args_size = 0;
  // reserve space for takum3 return value.
  if (return_type->name() == cdk::TYPE_TAKUM3) {
    _pf.INT(16);
    _pf.ALLOC();
  }
  // push normal arguments, right to left.
  if (node->arguments() && node->arguments()->size() > 0) {
    for (int ax = node->arguments()->size() - 1; ax >= 0; ax--) {
      auto arg = dynamic_cast<cdk::expression_node *>(node->arguments()->node(ax));
      auto expected = ftype->input(ax);
      arg->accept(this, lvl + 2);
      if (expected->name() == cdk::TYPE_TAKUM3 && arg->is_typed(cdk::TYPE_BALANCED3)) {
        _pf.B2T();
      }
      args_size += expected->size();
    }
  }

  // push hidden return pointer as first argument of callee.
  if (return_type->name() == cdk::TYPE_TAKUM3) {
    _pf.SP();
    if (args_size != 0) {
      _pf.INT(args_size);
      _pf.ADD();
    }
    args_size += 4;
  }
  _pf.CALL(node->identifier());
  if (args_size != 0) {
    _pf.TRASH(args_size);
  }
  if (return_type->name() == cdk::TYPE_BALANCED3) {
    _pf.LDFVAL64I();
  } else if (return_type->name() == cdk::TYPE_STRING || return_type->name() == cdk::TYPE_POINTER) {
    _pf.LDFVAL32I();
  } else if (return_type->name() == cdk::TYPE_VOID || return_type->name() == cdk::TYPE_TAKUM3) {
    // no load needed
  } else {
    // should not happen
  }
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_function_declaration_node(p6::function_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  auto function = new_symbol();
  _externals_to_declare.insert(function->name());
  reset_new_symbol();
}

//---------------------------------------------------------------------------

void p6::postfix_writer::do_function_definition_node(p6::function_definition_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _function = new_symbol();
  _externals_to_declare.erase(_function->name());
  reset_new_symbol();
  _returnSeen = false;

  // prepare arguments: positive offsets
  auto ftype = cdk::functional_type::cast(_function->type());
  auto return_type = ftype->output(0);
  _offset = (return_type->name() == cdk::TYPE_TAKUM3) ? 12 : 8;
  _symtab.push(); // scope of arguments
  if (node->arguments() && node->arguments()->size() > 0) {
    _inFunctionArgs = true;
    for (size_t ix = 0; ix < node->arguments()->size(); ix++) {
      cdk::basic_node *arg = node->arguments()->node(ix);
      if (arg == nullptr) break;
      arg->accept(this, lvl + 2);
    }
    _inFunctionArgs = false;
  }

  _pf.TEXT();
  _pf.ALIGN();
  if (node->qualifier() == tPUBLIC) {
    _pf.GLOBAL(_function->name(), _pf.FUNC());
  }
  _pf.LABEL(_function->name());

  // compute stack size for local variables
  p6::frame_size_calculator lsc(_compiler, _symtab);
  node->block()->accept(&lsc, lvl + 2);
  _pf.ENTER(lsc.localsize());

  // prepare local variables: negative offsets
  _offset = 0;
  _inFunctionBody = true;
  _globals.push(false);
  node->block()->accept(this, lvl + 4);
  _globals.pop();
  _inFunctionBody = false;

  // default epilogue if no explicit return was generated
  if (!_returnSeen) {
    _pf.LEAVE();
    _pf.RET();
  }
  _symtab.pop(); // scope of arguments
  _function = nullptr;
}

//---------------------------------------------------------------------------
// Teste prático

void p6::postfix_writer::do_conditional_iterate_node(p6::conditional_iterate_node * const node, int lvl) {

  ASSERT_SAFE_EXPRESSIONS;

  int lbl_loop = ++_lbl;
  int lbl_end = ++_lbl;

  auto ref = cdk::reference_type::cast(node->vector()->type());
  auto elem_type = ref->referenced();
  int elem_size = elem_type->size();

  // temporários locais:
  // vector pointer
  _offset -= 4;
  int vector_offset = _offset;

  // count convertido para int
  _offset -= 4;
  int count_offset = _offset;

  // índice i
  _offset -= 4;
  int index_offset = _offset;

  // unless condition:
  // se condition > 0, não executa o iterate  // if_node
  node->condition()->accept(this, lvl + 2);
  _pf.B2I();
  _pf.INT(0);
  _pf.JGT(mklbl(lbl_end));

  // guardar vector pointer
  node->vector()->accept(this, lvl + 2);
  _pf.LDINT();
  _pf.LOCAL(vector_offset);
  _pf.STINT();

  // guardar count
  node->count()->accept(this, lvl + 2);
  _pf.B2I();
  _pf.LOCAL(count_offset);
  _pf.STINT();

  // i = 0
  _pf.INT(0);
  _pf.LOCAL(index_offset);
  _pf.STINT();

  _pf.LABEL(mklbl(lbl_loop));

  // if i >= count goto end  
  _pf.LOCAL(index_offset);
  _pf.LDINT();

  _pf.LOCAL(count_offset);
  _pf.LDINT();

  _pf.LT();
  _pf.JZ(mklbl(lbl_end));

  // calcular endereço de vector[i] // index_node
  _pf.LOCAL(vector_offset);
  _pf.LDINT();

  _pf.LOCAL(index_offset);
  _pf.LDINT();

  _pf.INT(elem_size);
  _pf.MUL();
  _pf.ADD();

  // carregar vector[i]
  if (elem_type->name() == cdk::TYPE_BALANCED3) {
    _pf.LDBALANCED3();
  } else if (elem_type->name() == cdk::TYPE_TAKUM3) {
    _pf.LDTAKUM3();
  } else {
    _pf.LDINT();
  }

  // chamar função: using f
  _pf.CALL(node->identifier());

  // limpar argumento passado à função
  _pf.TRASH(elem_size);

  // i = i + 1
  _pf.LOCAL(index_offset);
  _pf.LDINT();
  _pf.INT(1);
  _pf.ADD();

  _pf.LOCAL(index_offset);
  _pf.STINT();

  _pf.JMP(mklbl(lbl_loop));

  _pf.LABEL(mklbl(lbl_end));
}