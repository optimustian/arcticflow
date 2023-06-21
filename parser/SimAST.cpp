#include "SimAST.h"

#include <cstring>

#include "sim_lexer.h"
#include "sim_parser.h"

namespace XPUSchedulerSimulator {

const char *src = nullptr;

std::string SimASTDumpIndent(int32_t indent) {
  std::string str;
  for (int i = 0; i < indent; i++) {
    str += " ";
  }
  return str;
}

SIMTranslationUnit *GenSimAST(const char *srcPtr) {
  src = srcPtr;
  yy_scan_bytes(srcPtr, strlen(srcPtr));
  SIMTranslationUnit *unit = nullptr;
  int status = yyparse(&unit);
  if (status == 0) {
    return unit;
  } else {
    return nullptr;
  }
}

std::string GetSrc(int32_t lineno) {
  std::string ret;
  lineno = std::max(lineno - 2, 0);
  int32_t currentLine = 0;
  for (const char *p = src; *p != 0; p++) {
    if (currentLine >= lineno && currentLine <= lineno + 2) {
      ret += *p;
    } else if (currentLine >= lineno + 2) {
      break;
    }
    if (*p == '\n') {
      currentLine++;
    }
  }

  return ret;
}

} // namespace XPUSchedulerSimulator