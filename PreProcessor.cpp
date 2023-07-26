#include "preAST.h"
#include "preLexer.h"
#include "preParser.h"
#include <iostream>
#include <string>

std::string src;
uint32_t g_lineNumber;

std::string commentFilt(const std::string &src) {
  int len = src.length();
  if (len < 2)
    return src;

  std::string filted;
  char c, prec;

  filted += src[0];

  bool isCommentBlock = false;
  for (int i = 1; i < len; i++) {
    if (!isCommentBlock) {
      filted += src[i];
      if (src[i] == '*' && src[i - 1] == '/') {
        isCommentBlock = true;
        filted.pop_back();
        filted.pop_back();
      } else if (src[i] == '/' && src[i - 1] == '/') {
        filted.pop_back();
        filted.pop_back();
        while (i + 1 < len && src[i + 1] != '\n') {
          i++;
        }
      }
    } else {
      if (src[i] == '/' && src[i - 1] == '*' && isCommentBlock) {
        isCommentBlock = false;
      }
    }
  }

  return filted;
}

std::string subreplace(std::string resource_str, std::string sub_str,
                       std::string new_str) {
  std::string dst_str = resource_str;
  std::string::size_type pos = 0;
  while ((pos = dst_str.find(sub_str)) != std::string::npos) //替换所有指定子串
  {
    dst_str.replace(pos, sub_str.length(), new_str);
  }
  return dst_str;
}

std::string flattenLine(const std::string &src) {
  std::string flattened = src;
  flattened = subreplace(flattened, "\\\n", "");
  flattened = subreplace(flattened, "\\\r\n", "");
  return flattened;
}

std::vector<std::string> stringSplitToVector(const std::string &src,
                                             const std::string &delim) {
  std::vector<std::string> res;
  if ("" == src)
    return res;
  char *strs = new char[src.length() + 1];
  strcpy(strs, src.c_str());

  char *d = new char[delim.length() + 1];
  strcpy(d, delim.c_str());

  char *p = strtok(strs, d);
  while (p) {
    std::string s = p;
    res.push_back(s);
    p = strtok(nullptr, d);
  }

  return res;
}

int main(int argc, char **argv) {
  std::string src = R"(
    #include "std\
io.h"
    /*
        COMMENT
    */
    #include <string.h>
    #include <bits/stdc++.h> // comments
    
    using namespace std;
    #include_next <iostream>
    
    #define VAL 0 what the fuck {\
    }

    #if 1 + 1 == 2
      content();
    #elif 0
    #elif 1
    #else
    #endif

    #ifdef a
      #ifndef b
        #else
      #endif
    #else
    #endif
    // jaja
    int a = 3;
    int main() {
      printf("hello, wor>ld! %d\n", a > 0);
    }
    // faf
  )";

  src = commentFilt(src);
  src = flattenLine(src);

  std::cout << src << std::endl;

  yy_scan_bytes(src.c_str(), src.length());
  TranslationUnit *unit = nullptr;
  src = "";
  yylval.src = &src;
  int status = yyparse(&unit);
  std::cout << "Parse Done " << status << std::endl;

  std::cout << ::src << std::endl;

  std::cout << "EXPR SIZE : " << unit->exprs.size() << std::endl;
  unit->dump();

  std::vector<std::string> srcVec = stringSplitToVector(::src, "\n");
  for (auto &s : srcVec) {
    std::cout << "! " << s << std::endl;
  }
}