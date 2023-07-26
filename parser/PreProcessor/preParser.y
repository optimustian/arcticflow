%parse-param {TranslationUnit **unit}

%code top {
    #include "preAST.h"
    #include "preParser.h"
    #include "stdio.h"

    extern int yylex(void);
    extern int yylineno;
    extern char *yytext;
    extern int yyinput(void);

    ParseState g_parseState;

    void yyerror(TranslationUnit **unit, const char* s) {
        fprintf(stderr, "Parse Error In Line %d\n", yylineno);
    }
}

%union {
    char *str;
    std::string *src;
    struct TranslationUnit *unit;
    struct PreProcessorExpr *expr;
    int isEOF;
}

%token PP_ONE PP_TWO INCLUDE INCLUDE_NEXT LINE DEFINE IF ELSE ELIF ENDIF IFDEF IFNDEF UNDEF PRAGMA WARNING ERROR
%token DOUBLE_QUOTE LT GT STRING_LITERAL LTGT_LITERAL SRC SYMBOL

%type<unit> translation_unit start
%type<expr> preprocessor_expr include_expr include_next_expr define_expr if_expr else_expr elif_expr endif_expr ifdef_expr ifndef_expr undef_expr source_expr

// Grammar rules

%%

start : translation_unit {
    $$ = $1;
}
;

translation_unit
    : preprocessor_expr {
        *unit = new TranslationUnit;
        if (dynamic_cast<IncludeExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<IncludeNextExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<DefineExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<IfdefExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<IfndefExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<IfExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<ElseExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<ElifExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<EndIfExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<UndefExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else if (dynamic_cast<SourceExpr *>($1)) {
            (*unit)->addPreProcessorExpr($1);
        } else {

        }
        $$ = *unit;
    }
    | translation_unit preprocessor_expr {
        TranslationUnit *unit = $$;
        $2->lineNo = g_lineNumber;
        if (dynamic_cast<IncludeExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<IncludeNextExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<DefineExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<IfdefExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<IfndefExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<IfExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<ElseExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<ElifExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<EndIfExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<UndefExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else if (dynamic_cast<SourceExpr *>($2)) {
            unit->addPreProcessorExpr($2);
        } else {

        }
        $$ = $1;
    }

preprocessor_expr
    : include_expr {
        printf("<preprocessor_expr:include_expr %u %p>\n", g_lineNumber, $1);
        $$ = $1;
    }
    | include_next_expr {
        printf("<preprocessor_expr:include_next_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | define_expr {
        printf("<preprocessor_expr:define_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | if_expr {
        printf("<preprocessor_expr:if_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | else_expr {
        printf("<preprocessor_expr:else_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | elif_expr {
        printf("<preprocessor_expr:elif_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | endif_expr {
        printf("<preprocessor_expr:endif_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | ifdef_expr {
        printf("<preprocessor_expr:ifdef_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | ifndef_expr {
        printf("<preprocessor_expr:ifndef_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
    | source_expr {
        printf("<preprocessor_expr:source_expr %u>\n", g_lineNumber);
        $$ = $1;
    }
define_expr
    : DEFINE SYMBOL{
        auto *expr = new DefineExpr;
        expr->name = yylval.str;
        printf("<#define [%s]>\n", yylval.str);
        char c;
        while ((c = getYYInput()) != 0) {
            if (c == '\n') {
                handleLineNumber();
                break;
            } else {
                expr->content += c;
                printf("~%c", c);
            }
        }
        $$ = expr;
    }

ifdef_expr
    : IFDEF SYMBOL {
        auto *expr = new IfdefExpr;
        expr->name = yylval.str;
        $$ = expr;
    }

ifndef_expr
    : IFNDEF SYMBOL {
        auto *expr = new IfndefExpr;
        expr->name = yylval.str;
        $$ = expr;
    }

undef_expr
    : UNDEF SYMBOL {
        auto *expr = new UndefExpr;
        expr->name = yylval.str;
        $$ = expr;
    }

if_expr
    : IF {
        auto *expr = new IfExpr;
        printf("<#if >\n");
        char c;
        while ((c = getYYInput()) != 0) {
            if (c == '\n') {
                handleLineNumber();
                break;
            } else {
                expr->expr += c;
                printf("~%c", c);
            }
        }
        $$ = expr;
    }

else_expr
    : ELSE {
        $$ = new ElseExpr;
    }

elif_expr
    : ELIF {
        auto *expr = new ElifExpr;
        printf("<#elif >\n");
        std::string exprStr;
        char c;
        while ((c = getYYInput()) != 0) {
            if (c == '\n') {
                handleLineNumber();
                break;
            } else {
                expr->expr += c;
                printf("~%c", c);
            }
        }

        $$ = expr;
    }

endif_expr
    : ENDIF {
        $$ = new EndIfExpr;
    }

source_expr
    : SRC {

    }

include_expr
    : INCLUDE STRING_LITERAL {
        auto *expr = new IncludeExpr;
        expr->path = yylval.str;
        $$ = expr;
    }
    | INCLUDE LTGT_LITERAL {
        auto *expr = new IncludeExpr;
        expr->path = yylval.str;
        $$ = expr;
    }

include_next_expr
    : INCLUDE_NEXT STRING_LITERAL {
        auto *expr = new IncludeNextExpr;
        expr->path = yylval.str;
        $$ = expr;
    }
    | INCLUDE_NEXT LTGT_LITERAL {
        auto *expr = new IncludeNextExpr;
        expr->path = yylval.str;
        $$ = expr;
    }
%%