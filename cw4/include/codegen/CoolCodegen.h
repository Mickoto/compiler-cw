#ifndef CODEGEN_COOL_CODEGEN_H_
#define CODEGEN_COOL_CODEGEN_H_

#include <memory>
#include <ostream>

#include "CoolParser.h"
#include "semantics/typed-ast/Classes.h"

class CoolCodegen {
  private:
    std::string file_name_;
    std::unique_ptr<Classes> ast;

  public:
    CoolCodegen(std::string file_name, std::unique_ptr<Classes> class_table)
        : file_name_(std::move(file_name)),
          ast(std::move(class_table)) {}

    void generate(std::ostream &out);
};

#endif
