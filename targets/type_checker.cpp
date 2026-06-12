#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"  // automatically generated
#include <cdk/types/primitive_type.h>
#include <cdk/types/types.h>

#define ASSERT_UNSPEC { if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) return; }

//---------------------------------------------------------------------------

void p6::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    cdk::basic_node *n = node->node(i);
    if (n) {
      n->accept(this, lvl + 2);
    }
  }
}

//---------------------------------------------------------------------------

void p6::type_checker::do_return_node(p6::return_node *const node, int lvl) {
  if (_current_function == nullptr) {
    throw std::string("return outside function");
  }

  auto ftype = cdk::functional_type::cast(_current_function->type());
  auto expected = ftype->output(0);

  if (node->expression() == nullptr) {
    if (expected->name() != cdk::TYPE_VOID) {
      throw std::string("return without value in non-void function.");
    }
    return;
  }

  if (expected->name() == cdk::TYPE_VOID) {
    throw std::string("return with value in void function.");
  }

  node->expression()->accept(this, lvl + 2);

  // infer unspecified return expressions from the function return type
  if (node->expression()->is_typed(cdk::TYPE_UNSPEC)) {
    if (expected->name() == cdk::TYPE_BALANCED3 ||  expected->name() == cdk::TYPE_TAKUM3 || expected->name() == cdk::TYPE_STRING) {
      node->expression()->type(expected);
    } else {
      throw std::string("cannot infer input type from return type.");
    }
  }
  if (expected->name() == cdk::TYPE_BALANCED3) {
    if (!node->expression()->is_typed(cdk::TYPE_BALANCED3)) {
      throw std::string("wrong return type (integer expected).");
    }
  } else if (expected->name() == cdk::TYPE_TAKUM3) {
    if (!node->expression()->is_typed(cdk::TYPE_BALANCED3) && !node->expression()->is_typed(cdk::TYPE_TAKUM3)) {
      throw std::string("wrong return type (integer or real expected).");
    }
  } else if (expected->name() == cdk::TYPE_STRING) {
    if (!node->expression()->is_typed(cdk::TYPE_STRING)) {
      throw std::string("wrong return type (string expected).");
    }
  } else if (expected->name() == cdk::TYPE_POINTER) {
    if (!node->expression()->is_typed(cdk::TYPE_POINTER)) {
      throw std::string("wrong return type (pointer expected).");
    }

    auto expected_ref = cdk::reference_type::cast(expected);
    auto actual_ref = cdk::reference_type::cast(node->expression()->type());

    auto expected_base = expected_ref->referenced();
    auto actual_base = actual_ref->referenced();

    // [void] pointer returns, like null, specialize to the expected pointer type
    if (actual_base->name() == cdk::TYPE_VOID) {
      node->expression()->type(expected);

    // non-void pointers must point to the same type
    } else if (expected_base->name() != cdk::TYPE_VOID &&
               cdk::to_string(expected_base) != cdk::to_string(actual_base)) {
      throw std::string("incompatible pointer return type");
    }
  } else {
    throw std::string("unknown return type.");
  }
}

//---------------------------------------------------------------------------

void p6::type_checker::do_nil_node(cdk::nil_node *const node, int lvl) {
  // EMPTY
}

void p6::type_checker::do_data_node(cdk::data_node *const node, int lvl) {
  // EMPTY
}

void p6::type_checker::do_block_node(p6::block_node *const node, int lvl) {
  _symtab.push();
  if (node->declarations()) node->declarations()->accept(this, lvl + 2);
  if (node->instructions()) node->instructions()->accept(this, lvl + 2);
  _symtab.pop();
}

void p6::type_checker::do_next_node(p6::next_node *const node, int lvl) {
  if (node->level() == 0 || node->level() > static_cast<size_t>(_in_loop)) {
    throw std::string("next outside of loop");
  }
}

void p6::type_checker::do_stop_node(p6::stop_node *const node, int lvl) {
  if (node->level() == 0 || node->level() > static_cast<size_t>(_in_loop)) {
    throw std::string("stop outside of loop");
  }
}

void p6::type_checker::do_double_node(cdk::double_node *const node, int lvl) {
  // EMPTY
}

void p6::type_checker::do_balanced3_node(cdk::balanced3_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_posit3_node(cdk::posit3_node *const node, int lvl) {
  // EMPTY
}

void p6::type_checker::do_takum3_node(cdk::takum3_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(16, cdk::TYPE_TAKUM3));
}

void p6::type_checker::do_not_node(cdk::not_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);

  if (node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    node->argument()->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
  }

  if (!node->argument()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("wrong type in unary logical expression");
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_and_node(cdk::and_node *const node, int lvl) {
  do_BooleanLogicalExpression(node, lvl);
}

void p6::type_checker::do_or_node(cdk::or_node *const node, int lvl) {
  do_BooleanLogicalExpression(node, lvl);
}

//---------------------------------------------------------------------------

void p6::type_checker::do_integer_node(cdk::integer_node *const node, int lvl) {
  // EMPTY
}

void p6::type_checker::do_string_node(cdk::string_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

void p6::type_checker::do_null_node(p6::null_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(0, cdk::TYPE_VOID)));
}

//---------------------------------------------------------------------------

void p6::type_checker::processUnaryExpression(cdk::unary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);

  if (node->argument()->is_typed(cdk::TYPE_BALANCED3) || node->argument()->is_typed(cdk::TYPE_TAKUM3)) {
    node->type(node->argument()->type());
  } else {
    throw std::string("wrong type in argument of unary expression");
  }
}

void p6::type_checker::do_unary_minus_node(cdk::unary_minus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

void p6::type_checker::do_unary_plus_node(cdk::unary_plus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

//protected:
void p6::type_checker::do_BooleanLogicalExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_BALANCED3) || !node->right()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("integer expression expected in logical expression");
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

//---------------------------------------------------------------------------

void p6::type_checker::do_add_node(cdk::add_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  // pointer + balanced3
  if (ltype == cdk::TYPE_POINTER && rtype == cdk::TYPE_BALANCED3) {
    node->type(node->left()->type());
    return;
  }

  // balanced3 + pointer
  if (ltype == cdk::TYPE_BALANCED3 && rtype == cdk::TYPE_POINTER) {
    node->type(node->right()->type());
    return;
  }

  // if one of the operands is takum3, the result is takum3; otherwise, it's balanced3
  if ((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
      (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)) {
    if (ltype == cdk::TYPE_TAKUM3 || rtype == cdk::TYPE_TAKUM3) {
      node->type(cdk::primitive_type::create(16, cdk::TYPE_TAKUM3));
    } else {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
    }
    return;
  }

  throw std::string("wrong types in add expression");
}

void p6::type_checker::do_sub_node(cdk::sub_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  // pointer - balanced3
  if (ltype == cdk::TYPE_POINTER && rtype == cdk::TYPE_BALANCED3) {
    node->type(node->left()->type());
    return;
  }

  // pointer - pointer
  if (ltype == cdk::TYPE_POINTER && rtype == cdk::TYPE_POINTER) {
    auto lref = cdk::reference_type::cast(node->left()->type());
    auto rref = cdk::reference_type::cast(node->right()->type());

    auto lbase = lref->referenced();
    auto rbase = rref->referenced();

    if (lbase->name() == cdk::TYPE_VOID || rbase->name() == cdk::TYPE_VOID || cdk::to_string(lbase) != cdk::to_string(rbase)) {
      throw std::string("incompatible pointer subtraction");
    }

    node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
    return;
  }

  // if one of the operands is takum3, the result is takum3; otherwise, it's balanced3
  if ((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
      (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)) {
    if (ltype == cdk::TYPE_TAKUM3 || rtype == cdk::TYPE_TAKUM3) {
      node->type(cdk::primitive_type::create(16, cdk::TYPE_TAKUM3));
    } else {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
    }
    return;
  }

  throw std::string("wrong types in sub expression");
}

void p6::type_checker::do_mul_node(cdk::mul_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
        (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)))
    throw std::string("wrong types in * expression");

  if (ltype == cdk::TYPE_TAKUM3 || rtype == cdk::TYPE_TAKUM3) {
    node->type(cdk::primitive_type::create(16, cdk::TYPE_TAKUM3));
  } else {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
  }
}

void p6::type_checker::do_div_node(cdk::div_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
        (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)))
    throw std::string("wrong types in div expression");

  if (ltype == cdk::TYPE_TAKUM3 || rtype == cdk::TYPE_TAKUM3) {
    node->type(cdk::primitive_type::create(16, cdk::TYPE_TAKUM3));
  } else {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
  }
}

void p6::type_checker::do_mod_node(cdk::mod_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (!node->left()->is_typed(cdk::TYPE_BALANCED3) || !node->right()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("wrong types in mod expression");
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_lt_node(cdk::lt_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
        (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)))
    throw std::string("wrong types in < expression");

  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_le_node(cdk::le_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
        (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)))
    throw std::string("wrong types in <= expression");

  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_ge_node(cdk::ge_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
        (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)))
    throw std::string("wrong types in >= expression");

  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_gt_node(cdk::gt_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
        (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)))
    throw std::string("wrong types in > expression");

  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_ne_node(cdk::ne_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!(((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
         (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)) ||
        (ltype == cdk::TYPE_POINTER && rtype == cdk::TYPE_POINTER))) {
    throw std::string("wrong types in != expression");
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

void p6::type_checker::do_eq_node(cdk::eq_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  auto ltype = node->left()->type()->name();
  auto rtype = node->right()->type()->name();

  if (!(((ltype == cdk::TYPE_BALANCED3 || ltype == cdk::TYPE_TAKUM3) &&
         (rtype == cdk::TYPE_BALANCED3 || rtype == cdk::TYPE_TAKUM3)) ||
        (ltype == cdk::TYPE_POINTER && rtype == cdk::TYPE_POINTER))) {
    throw std::string("wrong types in == expression");
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

//---------------------------------------------------------------------------

void p6::type_checker::do_variable_node(cdk::variable_node *const node, int lvl) {
  ASSERT_UNSPEC;
  const std::string &id = node->name();
  auto symbol = _symtab.find(id);

  if (symbol) {
    node->type(symbol->type());
  } else {
    throw std::string("undeclared variable '" + id + "'");
  }
}

void p6::type_checker::do_index_node(p6::index_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->base()->accept(this, lvl + 2);

  if (!node->base()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("pointer expression expected in index expression");
  }

  auto btype = cdk::reference_type::cast(node->base()->type());
  if (btype->referenced()->name() == cdk::TYPE_VOID) {
    throw std::string("cannot index pointer to void");
  }

  node->index()->accept(this, lvl + 2);
  if (!node->index()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("integer expression expected in index expression");
  }

  node->type(btype->referenced());
}

void p6::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl);
  node->type(node->lvalue()->type());
}

void p6::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl + 4);
  node->rvalue()->accept(this, lvl + 4);

  // infer unspecified rvalues from the lvalue type
  if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
    node->rvalue()->type(node->lvalue()->type());
  }

  if (node->lvalue()->is_typed(cdk::TYPE_BALANCED3)) {
    if (!node->rvalue()->is_typed(cdk::TYPE_BALANCED3)) {
      throw std::string("wrong assignment to integer");
    }
    node->type(node->lvalue()->type());
    return;
  }

  if (node->lvalue()->is_typed(cdk::TYPE_TAKUM3)) {
    if (!node->rvalue()->is_typed(cdk::TYPE_BALANCED3) && !node->rvalue()->is_typed(cdk::TYPE_TAKUM3)) {
      throw std::string("wrong assignment to real");
    }
    node->type(node->lvalue()->type());
    return;
  }

  if (node->lvalue()->is_typed(cdk::TYPE_STRING)) {
    if (!node->rvalue()->is_typed(cdk::TYPE_STRING)) {
      throw std::string("wrong assignment to string");
    }
    node->type(node->lvalue()->type());
    return;
  }

  if (node->lvalue()->is_typed(cdk::TYPE_POINTER)) {
    if (!node->rvalue()->is_typed(cdk::TYPE_POINTER)) {
      throw std::string("wrong assignment to pointer");
    }

    // get the referenced type of each pointer
    auto lref = cdk::reference_type::cast(node->lvalue()->type());
    auto rref = cdk::reference_type::cast(node->rvalue()->type());

    auto lbase = lref->referenced();
    auto rbase = rref->referenced();

    // [void] pointers like null and stack_alloc specialize to the lvalue pointer type
    if (rbase->name() == cdk::TYPE_VOID) {
      node->rvalue()->type(node->lvalue()->type());
      node->type(node->lvalue()->type());
      return;
    }

    if (lbase->name() != cdk::TYPE_VOID && cdk::to_string(lbase) != cdk::to_string(rbase)) {
      throw std::string("incompatible pointer assignment");
    }
    node->type(node->lvalue()->type());
    return;
  }

  throw std::string("wrong types in assignment");
}

//---------------------------------------------------------------------------

void p6::type_checker::do_variable_declaration_node(p6::variable_declaration_node *const node, int lvl) {
  if (node->initializer() != nullptr) {
    if (node->qualifier() == tFORWARD) {
      throw std::string("forward variable declaration cannot have initializer.");
    }
    node->initializer()->accept(this, lvl + 2);

    // infer auto declarations from the initializer type (auto x = 3)
    if (node->type() == nullptr) {
      if (node->initializer()->is_typed(cdk::TYPE_UNSPEC)) {
        throw std::string("cannot infer type from initializer");
      }
      node->type(node->initializer()->type());
    }

    // unspecified initializers use the declared variable type (int x = input)
    if (node->initializer()->is_typed(cdk::TYPE_UNSPEC)) {
      node->initializer()->type(node->type());
    }

    if (node->is_typed(cdk::TYPE_BALANCED3)) {
      if (!node->initializer()->is_typed(cdk::TYPE_BALANCED3)) {
        throw std::string("wrong type for initializer (integer expected).");
      }
    } else if (node->is_typed(cdk::TYPE_TAKUM3)) {
      if (!node->initializer()->is_typed(cdk::TYPE_BALANCED3) && !node->initializer()->is_typed(cdk::TYPE_TAKUM3)) {
        throw std::string("wrong type for initializer (integer or real expected).");
      }
    } else if (node->is_typed(cdk::TYPE_STRING)) {
      if (!node->initializer()->is_typed(cdk::TYPE_STRING)) {
        throw std::string("wrong type for initializer (string expected).");
      }
    } else if (node->is_typed(cdk::TYPE_POINTER)) {
      if (!node->initializer()->is_typed(cdk::TYPE_POINTER)) {
        throw std::string("wrong type for initializer (pointer expected).");
      }

      auto lref = cdk::reference_type::cast(node->type());
      auto rref = cdk::reference_type::cast(node->initializer()->type());

      auto lbase = lref->referenced();
      auto rbase = rref->referenced();

      if (rbase->name() == cdk::TYPE_VOID) {
        node->initializer()->type(node->type());
      } else if (lbase->name() != cdk::TYPE_VOID && cdk::to_string(lbase) != cdk::to_string(rbase)) {
        throw std::string("incompatible pointer initializer");
      }
    } else {
      throw std::string("unknown type for initializer.");
    }
  }

  if (node->type() == nullptr) {
    throw std::string("declaration requires type or initializer.");
  }

  const std::string &id = node->identifier();
  auto symbol = p6::make_symbol(node->qualifier(), node->type(), id, false, node->qualifier() != tFORWARD);
  if (_symtab.insert(id, symbol)) {
    _parent->set_new_symbol(symbol);
  } else {
    throw std::string("variable '" + id + "' redeclared");
  }
}

//---------------------------------------------------------------------------

void p6::type_checker::do_program_node(p6::program_node *const node, int lvl) {
  _current_function = p6::make_symbol(node->qualifier(), node->type(), "", true, true);
  node->block()->accept(this, lvl + 2);
  _current_function = nullptr;
}

void p6::type_checker::do_evaluation_node(p6::evaluation_node *const node, int lvl) {
  node->argument()->accept(this, lvl + 2);
}

void p6::type_checker::do_print_node(p6::print_node *const node, int lvl) {
  node->arguments()->accept(this, lvl + 2);

  for (size_t i = 0; i < node->arguments()->size(); i++) {
    auto *arg = dynamic_cast<cdk::expression_node *>(node->arguments()->node(i));

    if (!arg->is_typed(cdk::TYPE_BALANCED3) && !arg->is_typed(cdk::TYPE_TAKUM3) && !arg->is_typed(cdk::TYPE_STRING)) {
      throw std::string("wrong type in print argument");
    }
  }
}

//---------------------------------------------------------------------------

void p6::type_checker::do_sizeof_node(p6::sizeof_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->expression()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(8, cdk::TYPE_BALANCED3));
}

//---------------------------------------------------------------------------

void p6::type_checker::do_input_node(p6::input_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC));
}

//---------------------------------------------------------------------------

void p6::type_checker::do_address_of_node(p6::address_of_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl + 2);
  node->type(cdk::reference_type::create(4, node->lvalue()->type()));
}

void p6::type_checker::do_stack_alloc_node(p6::stack_alloc_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);

  if (!node->argument()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("integer expression expected in allocation expression");
  }
  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(0, cdk::TYPE_VOID)));
}

//---------------------------------------------------------------------------

void p6::type_checker::do_while_node(p6::while_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("expected integer condition");
  }
  _in_loop++;
  node->block()->accept(this, lvl + 4);
  _in_loop--;
}

//---------------------------------------------------------------------------

void p6::type_checker::do_if_node(p6::if_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("expected integer condition");
  }
  node->block()->accept(this, lvl + 4);
}

void p6::type_checker::do_if_else_node(p6::if_else_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("expected integer condition");
  }
  node->thenblock()->accept(this, lvl + 4);
  node->elseblock()->accept(this, lvl + 4);
}

//---------------------------------------------------------------------------

void p6::type_checker::do_function_call_node(p6::function_call_node *const node, int lvl) {
  ASSERT_UNSPEC;
  const std::string &id = node->identifier();
  auto symbol = _symtab.find(id);
  if (symbol == nullptr) {
    throw std::string("symbol '" + id + "' is undeclared.");
  }
  if (!symbol->is_function()) {
    throw std::string("symbol '" + id + "' is not a function.");
  }
  auto ftype = cdk::functional_type::cast(symbol->type());
  if (node->arguments()->size() != ftype->input()->length()) {
    throw std::string("wrong number of arguments in function call.");
  }
  node->arguments()->accept(this, lvl + 4);
  for (size_t i = 0; i < node->arguments()->size(); i++) {
    auto arg = node->argument(i);
    auto expected = ftype->input(i);

    if (arg->is_typed(cdk::TYPE_UNSPEC)) {
      if (expected->name() == cdk::TYPE_BALANCED3 || expected->name() == cdk::TYPE_TAKUM3 || expected->name() == cdk::TYPE_STRING) {
        arg->type(expected);
      }
    }
    if (cdk::to_string(arg->type()) == cdk::to_string(expected)) {
      continue;
    }
    if (expected->name() == cdk::TYPE_TAKUM3 && arg->is_typed(cdk::TYPE_BALANCED3)) {
      continue;
    }
    if (expected->name() == cdk::TYPE_POINTER && arg->is_typed(cdk::TYPE_POINTER)) {
      auto expected_ref = cdk::reference_type::cast(expected);
      auto actual_ref = cdk::reference_type::cast(arg->type());

      auto expected_base = expected_ref->referenced();
      auto actual_base = actual_ref->referenced();

      // [void] pointer arguments specialize to the expected pointer type
      if (actual_base->name() == cdk::TYPE_VOID) {
        arg->type(expected);
        continue;
      }

      // [void] parameters accept any pointer; otherwise pointed types must match
      if (expected_base->name() == cdk::TYPE_VOID || cdk::to_string(expected_base) == cdk::to_string(actual_base)) {
        continue;
      }
    }
    throw std::string("type mismatch for argument " + std::to_string(i + 1) + " of '" + id + "'.");
  }
  node->type(ftype->output(0));
}

//---------------------------------------------------------------------------

void p6::type_checker::do_function_declaration_node(p6::function_declaration_node *const node, int lvl) {
  const std::string &id = node->identifier();
  if (!node->is_typed(cdk::TYPE_FUNCTIONAL)) {
    throw std::string("function declaration must have functional type");
  }

  // declaration creates a function symbol that is not defined yet
  auto symbol = p6::make_symbol(node->qualifier(), node->type(), id, true, false);
  if (_symtab.insert(id, symbol)) {
    _parent->set_new_symbol(symbol);
  } else {
    auto previous = _symtab.find(id);

    if (!previous || !previous->is_function()) {
      throw std::string("'" + id + "' already declared as variable");
    }
    if (cdk::to_string(previous->type()) != cdk::to_string(node->type())) {
      throw std::string("conflicting type for function '" + id + "'");
    }
    throw std::string("function '" + id + "' redeclared");
  }
}

//---------------------------------------------------------------------------

void p6::type_checker::do_function_definition_node(p6::function_definition_node *const node, int lvl) {
  const std::string &id = node->identifier();
  if (!node->is_typed(cdk::TYPE_FUNCTIONAL)) {
    throw std::string("function definition must have functional type");
  }

  auto function_type = node->type();
  auto symbol = p6::make_symbol(node->qualifier(), function_type, id, true, true);
  // insert new function or replace a compatible forward declaration
  if (!_symtab.insert(id, symbol)) {
    auto previous = _symtab.find(id);

    if (!previous || !previous->is_function()) {
      throw std::string("'" + id + "' already declared as variable");
    }
    if (previous->defined()) {
      throw std::string("function '" + id + "' already defined");
    }
    if (cdk::to_string(previous->type()) != cdk::to_string(function_type)) {
      throw std::string("conflicting type for function '" + id + "'");
    }
    _symtab.replace(id, symbol);
  }

  auto previous_function = _current_function;
  _current_function = symbol;
  // function arguments live in the function scope
  _symtab.push();

  if (node->arguments()) {
    node->arguments()->accept(this, lvl + 2);
  }
  if (node->block()) {
    node->block()->accept(this, lvl + 2);
  }

  _symtab.pop();
  _current_function = previous_function;
  _parent->set_new_symbol(symbol);
}

//---------------------------------------------------------------------------
// Teste prático

void p6::type_checker::do_conditional_iterate_node(p6::conditional_iterate_node *const node, int lvl) {

  node->condition()->accept(this, lvl + 2);
  if (!node->condition()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("expected integer condition");
  }

  node->vector()->accept(this, lvl + 2);
  if (!node->vector()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("expected vector pointer");
  }

  auto vtype = cdk::reference_type::cast(node->vector()->type());
  auto elem_type = vtype->referenced();

  if (elem_type->name() == cdk::TYPE_VOID) {
    throw std::string("cannot iterate vector of void");
  }

  node->count()->accept(this, lvl + 2);
  if (!node->count()->is_typed(cdk::TYPE_BALANCED3)) {
    throw std::string("expected integer count");
  }

  const std::string &id = node->identifier();
  auto symbol = _symtab.find(id);

  if (symbol == nullptr) {
    throw std::string("symbol '" + id + "' is undeclared.");
  }

  if (!symbol->is_function()) {
    throw std::string("symbol '" + id + "' is not a function.");
  }

  auto ftype = cdk::functional_type::cast(symbol->type());

  if (ftype->input()->length() != 1) {
    throw std::string("function used in iterate must take one argument");
  }

  if (cdk::to_string(ftype->input(0)) != cdk::to_string(elem_type)) {
    throw std::string("incompatible vector element type");
  }

  auto expected = ftype->output(0);

  if (expected->name() != cdk::TYPE_VOID) {
    throw std::string("incompatible return type");
  }
}