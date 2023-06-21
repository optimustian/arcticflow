#ifndef __SIM_AST2IR_H_
#define __SIM_AST2IR_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "SimAST.h"

namespace XPUSchedulerSimulator {

struct SIMIRBuilder {
  std::string ir;

  void AST2CPPIR(SIMTranslationUnit *unit);
  std::string EmitIRHeader();
  std::string EmitHardwareEnum(SIMTranslationUnit *unit);
  std::string EmitHardwareCntMap(SIMTranslationUnit *unit);
  std::string EmitOperatorFuncBody(SIMTranslationUnit *unit);
  std::string EmitOpToTimeMap(SIMTranslationUnit *unit);
  std::string EmitInstanceMap(SIMTranslationUnit *unit);
  std::string EmitRegisterInstanceFunc(SIMTranslationUnit *unit);
  std::string EmitFlowFunc(SIMTranslationUnit *unit);
  std::string EmitSimuFunc(SIMTranslationUnit *unit);
  std::string EmitSpinLockClass(SIMTranslationUnit *unit);
  std::string EmitSimpleScheduler(SIMTranslationUnit *unit);
  std::string EmitGreedyScheduler(SIMTranslationUnit *unit);
  std::string EmitInstanceExecuteService(SIMTranslationUnit *unit);
  std::string EmitMainFunc(SIMTranslationUnit *unit);
};

} // namespace XPUSchedulerSimulator

#endif