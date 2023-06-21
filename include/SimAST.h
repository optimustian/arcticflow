#ifndef __SIM_AST_H_
#define __SIM_AST_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

template <typename... Args>
std::string StringFormat(const char *format, Args... args) {
  size_t length = std::snprintf(nullptr, 0, format, args...);
  if (length <= 0) {
    return "";
  }

  char *buf = new char[length + 1];
  std::snprintf(buf, length + 1, format, args...);

  std::string str(buf);
  delete[] buf;
  return str;
}
namespace XPUSchedulerSimulator {

std::string SimASTDumpIndent(int32_t indent = 0);

struct SIMSymbol {
  std::string name;
  virtual ~SIMSymbol() {}
};

struct SIMFlowExpression {
  template <typename T>
  T as() {
    return dynamic_cast<T>(this);
  }
  virtual std::string dump(int32_t indent = 0) { return ""; }
  virtual ~SIMFlowExpression() {}
};

struct SIMFlowUnaryExpression : SIMFlowExpression {
  SIMSymbol *opName;
  std::string dump(int32_t indent = 0) override {
    return StringFormat("%s{SIMFlowUnaryExpression %s}\n",
                        SimASTDumpIndent(indent).c_str(), opName->name.c_str());
  }
  virtual ~SIMFlowUnaryExpression() override {}
};

struct SIMFlowBinaryExpression : SIMFlowExpression {
  SIMFlowExpression *leftExpr;
  SIMFlowExpression *rightExpr;
  std::string dump(int32_t indent = 0) override {
    std::string ret = StringFormat("%s{SIMFlowBinaryExpression\n",
                                   SimASTDumpIndent(indent).c_str());
    ret += StringFormat("%s", leftExpr->dump(indent + 2).c_str());
    ret += StringFormat("%s", rightExpr->dump(indent + 2).c_str());
    ret += StringFormat("%s}\n", SimASTDumpIndent(indent).c_str());
    return ret;
  }
  virtual ~SIMFlowBinaryExpression() override {
    delete leftExpr;
    delete rightExpr;
  }
};

struct SIMBlock {
  template <typename T>
  T as() {
    return dynamic_cast<T>(this);
  }
  virtual std::string dump(int32_t indent = 0) { return ""; }
  virtual ~SIMBlock() {}
};

struct SIMFlowBlock : SIMBlock {
  std::vector<SIMFlowExpression *> exprs;
  std::string dump(int32_t indent = 0) override {
    std::string ret =
        StringFormat("%s{SIMFlowBlock\n", SimASTDumpIndent(indent).c_str());
    for (auto *expr : exprs) {
      ret += StringFormat("%s", expr->dump(indent + 2).c_str());
    }
    ret += StringFormat("%s}\n", SimASTDumpIndent(indent).c_str());
    return ret;
  }
  ~SIMFlowBlock() {
    for (auto *expr : exprs) {
      delete expr;
    }
  }
};

struct SIMExpression {
  template <typename T>
  T as() {
    return dynamic_cast<T>(this);
  }
  virtual std::string dump(int32_t indent = 0) { return ""; }
  virtual ~SIMExpression() {}
};

struct SIMCallExpression : SIMExpression {
  SIMSymbol *name;
  int32_t arg0;
  std::string dump(int32_t indent = 0) override {
    if (name->name == "sleep") {
      return StringFormat("%s{SIMCallExpression %s %d}\n",
                          SimASTDumpIndent(indent).c_str(), name->name.c_str(),
                          arg0);
    } else {
      return StringFormat("%s{SIMCallExpression %s}\n",
                          SimASTDumpIndent(indent).c_str(), name->name.c_str());
    }
  }
  virtual ~SIMCallExpression() { delete name; }
};

struct SIMSimuBlock : SIMBlock {
  std::vector<SIMExpression *> exprs;
  std::string dump(int32_t indent = 0) override {
    std::string ret =
        StringFormat("%s{SIMSimuBlock\n", SimASTDumpIndent(indent).c_str());
    for (auto *expr : exprs) {
      ret += StringFormat("%s", expr->dump(indent + 2).c_str());
    }
    ret += StringFormat("%s}\n", SimASTDumpIndent(indent).c_str());
    return ret;
  }
  ~SIMSimuBlock() {
    for (auto *expr : exprs) {
      delete expr;
    }
  }
};

struct SIMHardwareExpr {
  SIMSymbol *hardwareName;
  int32_t hardwareCnt;
  std::string dump(int32_t indent = 0) {
    return StringFormat("%s{SIMHardwareExpr %s %d}\n",
                        SimASTDumpIndent(indent).c_str(),
                        hardwareName->name.c_str(), hardwareCnt);
  }
  ~SIMHardwareExpr() { delete hardwareName; }
};

struct SIMHardwareBlock : SIMBlock {
  std::vector<SIMHardwareExpr *> exprs;
  std::string dump(int32_t indent = 0) override {
    std::string ret =
        StringFormat("%s{SIMHardwareBlock\n", SimASTDumpIndent(indent).c_str());
    for (auto *expr : exprs) {
      ret += StringFormat("%s  %s", SimASTDumpIndent(indent).c_str(),
                          expr->dump().c_str());
    }
    ret += StringFormat("%s}\n", SimASTDumpIndent(indent).c_str());
    return ret;
  }
  ~SIMHardwareBlock() {
    for (auto *expr : exprs) {
      delete expr;
    }
  }
};

struct SIMOperatorExpr {
  SIMSymbol *opName;
  SIMSymbol *hardwareName;
  int32_t time;
  std::string dump(int32_t indent = 0) {
    return StringFormat("%s{SIMOperatorExpr %s %s %d}\n",
                        SimASTDumpIndent(indent).c_str(), opName->name.c_str(),
                        hardwareName->name.c_str(), time);
  }
  ~SIMOperatorExpr() {
    delete opName;
    delete hardwareName;
  }
};

struct SIMOperatorBlock : SIMBlock {
  std::vector<SIMOperatorExpr *> exprs;
  std::string dump(int32_t indent = 0) override {
    std::string ret =
        StringFormat("%s{SIMOperatorBlock\n", SimASTDumpIndent(indent).c_str());
    for (auto *expr : exprs) {
      ret += StringFormat("%s  %s", SimASTDumpIndent(indent).c_str(),
                          expr->dump().c_str());
    }
    ret += StringFormat("%s}\n", SimASTDumpIndent(indent).c_str());
    return ret;
  }
  ~SIMOperatorBlock() {
    for (auto *expr : exprs) {
      delete expr;
    }
  }
};

struct SIMForeachExpression : SIMExpression, SIMFlowExpression {
  int32_t loopCnt;
  SIMBlock *loopBlock = nullptr;
  std::string dump(int32_t indent = 0) override {
    if (loopBlock == nullptr) {
      return "";
    }
    std::string ret = StringFormat("%s{SIMForeachExpression %d\n",
                                   SimASTDumpIndent(indent).c_str(), loopCnt);
    ret += StringFormat("%s", loopBlock->dump(indent + 2).c_str());
    ret += StringFormat("%s}\n", SimASTDumpIndent(indent).c_str());
    return ret;
  }
  virtual ~SIMForeachExpression() override { delete loopBlock; }
};

struct SIMTranslationUnit {
  SIMHardwareBlock *hardware;
  SIMOperatorBlock *op;
  SIMSimuBlock *simu;
  std::map<SIMSymbol *, SIMFlowBlock *> flowBlocks;

  std::string dump(int32_t indent = 0) {
    std::string str;
    str += hardware->dump(indent);
    str += op->dump(indent);
    str += simu->dump(indent);
    str += std::string("{FlowBlocks\n");
    for (auto &[sym, block] : flowBlocks) {
      str += sym->name + std::string(":\n");
      str += block->dump(indent + 2);
    }
    str += std::string("}");
    return str;
  }

  ~SIMTranslationUnit() {
    delete hardware;
    delete op;
    delete simu;
    for (auto &[sym, block] : flowBlocks) {
      delete sym;
      delete block;
    }
  }
};

extern const char *src;

SIMTranslationUnit *GenSimAST(const char *srcPtr);

std::string GetSrc(int32_t lineno);
};  // namespace XPUSchedulerSimulator
#endif