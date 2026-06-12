#pragma once

#include <memory>
#include <cdk/yy_factory.h>
#include "p6_scanner.h"

namespace p6 {

  /**
   * This class implements the compiler factory for the P6 compiler.
   */
  class factory: public cdk::yy_factory<p6_scanner> {
    /**
     * This object is automatically registered by the constructor in the
     * superclass' language registry.
     */
    static factory _self;

  protected:
    /**
     * @param language name of the language handled by this factory (see .cpp file)
     */
    factory(const std::string &language = "p6") :
        cdk::yy_factory<p6_scanner>(language) {
    }

  };

} // p6

