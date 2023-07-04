#include "preAST.h"
#include "preLexer.h"
#include "preParser.h"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  std::cout << "Hello PreProcessor" << std::endl;

  std::string src = R"(
    #include "stdio.h"
    #include <string.h>
    #include <bits/stdc++.h>
  )";

  yy_scan_bytes(src.c_str(), src.length());
  TranslationUnit *unit = nullptr;
  int status = yyparse(&unit);
  std::cout << "Parse Done " << status << std::endl;
}