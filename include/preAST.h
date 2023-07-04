#ifndef __PRE_PROCESSOR_AST_H_
#define __PRE_PROCESSOR_AST_H_

#include <string>
#include <vector>

struct PreProcessorExpr {
  virtual void dump() {}
};

struct SourceExpr : PreProcessorExpr {
  std::string src;

  void dump(){};
};

struct IncludeExpr : PreProcessorExpr {
  std::string path;

  void dump(){};
};

struct IncludeNextExpr : PreProcessorExpr {
  std::string path;

  void dump(){};
};

struct DefineExpr : PreProcessorExpr {
  std::string name;
  std::string content;

  void dump() override{};
};

struct TranslationUnit {
  std::vector<PreProcessorExpr *> exprs;
};

#endif