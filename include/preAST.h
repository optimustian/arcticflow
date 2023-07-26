#ifndef __PRE_PROCESSOR_AST_H_
#define __PRE_PROCESSOR_AST_H_

#include <iostream>
#include <string>
#include <vector>

extern int getYYInput();
extern int handleLineNumber();

extern uint32_t g_lineNumber;
extern std::string src;

struct ParseState {
  bool isEOF;
  bool isPreProcessorLine;
};

extern ParseState g_parseState;

struct PreProcessorExpr {
  uint32_t lineNo;
  virtual void dump() {}
};

struct SourceExpr : PreProcessorExpr {
  std::string src;

  void dump(){};
};

struct IncludeExpr : PreProcessorExpr {
  std::string path;

  void dump() {
    std::cout << lineNo << ": "
              << "IncludeExpr: " << path << std::endl;
  };
};

struct IncludeNextExpr : PreProcessorExpr {
  std::string path;

  void dump() {
    std::cout << lineNo << ": "
              << "IncludeNextExpr: " << path << std::endl;
  };
};

struct DefineExpr : PreProcessorExpr {
  std::string name;
  std::string content;

  void dump() {
    std::cout << lineNo << ": "
              << "DefineExpr: " << name << " " << content << std::endl;
  };
};

struct IfdefExpr : PreProcessorExpr {
  std::string name;

  void dump() {
    std::cout << lineNo << ": "
              << "IfdefExpr: " << name << std::endl;
  };
};

struct IfndefExpr : PreProcessorExpr {
  std::string name;

  void dump() {
    std::cout << lineNo << ": "
              << "IfndefExpr: " << name << std::endl;
  };
};

struct IfExpr : PreProcessorExpr {
  std::string expr;

  void dump() {
    std::cout << lineNo << ": "
              << "IfExpr: " << expr << std::endl;
  };
};

struct ElifExpr : PreProcessorExpr {
  std::string expr;

  void dump() {
    std::cout << lineNo << ": "
              << "ElifExpr: " << expr << std::endl;
  };
};

struct ElseExpr : PreProcessorExpr {
  void dump() {
    std::cout << lineNo << ": "
              << "ElseExpr" << std::endl;
  };
};

struct EndIfExpr : PreProcessorExpr {
  void dump() {
    std::cout << lineNo << ": "
              << "EndIfExpr" << std::endl;
  };
};

struct UndefExpr : PreProcessorExpr {
  std::string name;

  void dump() {
    std::cout << lineNo << ": "
              << "UndefExpr: " << name << std::endl;
  };
};

struct TranslationUnit {
  std::vector<PreProcessorExpr *> exprs;

  void addPreProcessorExpr(PreProcessorExpr *expr) { exprs.push_back(expr); }

  void dump() {
    for (auto &expr : exprs) {
      expr->dump();
    }
  }
};

#endif