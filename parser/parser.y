%parse-param {XPUSchedulerSimulator::SIMTranslationUnit **unit}

%code top {
    #include "SimAST.h"
    #include <iostream>

    using namespace XPUSchedulerSimulator;
    extern int yylex(void);
    extern int yylineno;

    struct SIMSymbol *g_FlowBlockLeftSymbol;

    static void yyerror(XPUSchedulerSimulator::SIMTranslationUnit **unit, const char* s) {
        fprintf(stderr, "Parse Error In Line %d\n", yylineno);
        fprintf(stderr, "======= SRC =======\n");
        fprintf(stderr, "%s", GetSrc(yylineno).c_str());
        fprintf(stderr, "===================\n");
        if (*unit != nullptr) {
            delete *unit;
        }
    }
}

%union {
    char *str;
    int iVal;
    double fVal;
    struct SIMSymbol *symbol;
    struct SIMFlowExpression *arrowExpr;
    struct SIMExpression *simExpr;
    struct SIMOperatorExpr *opDeclExpr;
    struct SIMHardwareExpr *hardwareDeclExpr;
    struct SIMBlock *block;
    struct SIMTranslationUnit *unit;
}

// Terminals

%token HARDWARE OPERATOR SIMU FOREACH SLEEP ASSIGN LEFT_BIG_PAR RIGHT_BIG_PAR LEFT_SMALL_PAR RIGHT_SMALL_PAR
%token LEFT_MID_PAR RIGHT_MID_PAR COMMA I_CONSTANT F_CONSTANT SEMI SYMBOL

// Precedence and associativity

%left ARROW
%left DOT

%type<str> SYMBOL
%type<iVal> constantExpr
%type<symbol> varExpr
%type<arrowExpr> flowDeclarator arrowExpr flowForeachExpr
%type<simExpr> simuDeclarator simuExpr
%type<opDeclExpr> operatorDeclarator
%type<hardwareDeclExpr> hardwareDeclarator
%type<block> simuBlock simuDeclaratorList
%type<block> operatorBlock operatorDeclaratorList
%type<block> hardwareBlock hardwareDeclaratorList
%type<block> block flowBlock flowDeclaratorList
%type<unit> start translationUnit

%%

// Grammar rules

start
    : translationUnit {
        *unit = $1;
        $$ = $1;
    }
;

translationUnit
    : block {
        SIMTranslationUnit *unit = new SIMTranslationUnit;
        if (SIMHardwareBlock *hardwareBlock = dynamic_cast<SIMHardwareBlock *>($1)) {
            unit->hardware = hardwareBlock;
        } else if (SIMOperatorBlock *operatorBlock = dynamic_cast<SIMOperatorBlock *>($1)) {
            unit->op = operatorBlock;
        } else if (SIMSimuBlock *simuBlock = dynamic_cast<SIMSimuBlock *>($1)) {
            unit->simu = simuBlock;
        } else if (SIMFlowBlock *flowBlock = dynamic_cast<SIMFlowBlock *>($1)) {
            unit->flowBlocks.emplace(g_FlowBlockLeftSymbol, flowBlock);
        }
        $$ = unit;
    }
    | translationUnit block {
        if (SIMHardwareBlock *hardwareBlock = dynamic_cast<SIMHardwareBlock *>($2)) {
            $1->hardware = hardwareBlock;
        } else if (SIMOperatorBlock *operatorBlock = dynamic_cast<SIMOperatorBlock *>($2)) {
            $1->op = operatorBlock;
        } else if (SIMSimuBlock *simuBlock = dynamic_cast<SIMSimuBlock *>($2)) {
            $1->simu = simuBlock;
        } else if (SIMFlowBlock *flowBlock = dynamic_cast<SIMFlowBlock *>($2)) {
            $1->flowBlocks.emplace(g_FlowBlockLeftSymbol, flowBlock);
        }
        $$ = $1;
    }
;

block
    : hardwareBlock {
        $$ = $1;
    }
    | operatorBlock {
        $$ = $1;
    }
    | flowBlock {
        $$ = $1;
    }
    | simuBlock {
        $$ = $1;
    }
;

simuBlock
    : SIMU ASSIGN LEFT_BIG_PAR simuDeclaratorList RIGHT_BIG_PAR SEMI {
        $$ = $4;
    }
;

simuDeclaratorList
    : simuDeclarator {
        SIMSimuBlock *block = new SIMSimuBlock;
        block->exprs.emplace_back($1);
        $$ = block;
    }
    | simuDeclaratorList simuDeclarator {
        dynamic_cast<SIMSimuBlock *>($1)->exprs.emplace_back($2);
        $$ = $1;
    }
;

simuDeclarator
    : simuExpr SEMI {
        $$ = $1;
    }
    | SEMI {
        SIMCallExpression *expr = new SIMCallExpression;
        expr->name = nullptr;
        expr->arg0 = 0;
        $$ = expr;
    }
    |  FOREACH LEFT_SMALL_PAR constantExpr RIGHT_SMALL_PAR LEFT_BIG_PAR simuDeclaratorList RIGHT_BIG_PAR {
        SIMForeachExpression *expr = new SIMForeachExpression;
        expr->loopCnt = $3;
        expr->loopBlock = dynamic_cast<SIMSimuBlock *>($6);
        $$ = expr;
    }
;

simuExpr
    : varExpr LEFT_SMALL_PAR RIGHT_SMALL_PAR {
        SIMCallExpression *expr = new SIMCallExpression;
        expr->name = $1;
        expr->arg0 = 0;
        $$ = expr;
    }
    | SLEEP LEFT_SMALL_PAR constantExpr RIGHT_SMALL_PAR {
        SIMCallExpression *expr = new SIMCallExpression;
        expr->name = new SIMSymbol;
        expr->name->name = std::string("sleep");
        expr->arg0 = $3;
        $$ = expr;
    }
;

flowBlock
    : varExpr ASSIGN LEFT_BIG_PAR flowDeclaratorList RIGHT_BIG_PAR SEMI {
        g_FlowBlockLeftSymbol = $1;
        $$ = $4;
    }
;

flowDeclaratorList
    : flowDeclarator {
        SIMFlowBlock *block = new SIMFlowBlock;
        block->exprs.emplace_back($1);
        $$ = block;
    }
    | flowDeclaratorList flowDeclarator {
        dynamic_cast<SIMFlowBlock *>($1)->exprs.emplace_back($2);
        $$ = $1;
    }
;

flowDeclarator
    : arrowExpr SEMI {
        $$ = $1;
    }
;

arrowExpr
    : varExpr {
        SIMFlowUnaryExpression *expr = new SIMFlowUnaryExpression;
        expr->opName = $1;
        $$ = expr;
    }
    | arrowExpr ARROW varExpr {
        SIMFlowBinaryExpression *expr = new SIMFlowBinaryExpression;
        expr->leftExpr = $1;
        SIMFlowUnaryExpression *rightExpr = new SIMFlowUnaryExpression;
        rightExpr->opName = $3;
        expr->rightExpr = rightExpr;
        $$ = expr;
    }
    | arrowExpr ARROW flowForeachExpr {
        SIMFlowBinaryExpression *expr = new SIMFlowBinaryExpression;
        expr->leftExpr = $1;
        expr->rightExpr = $3;
        $$ = expr;
    }
    | flowForeachExpr {
        SIMForeachExpression *expr = new SIMForeachExpression;
        expr = (SIMForeachExpression *)$1;
        $$ = expr;
    }
;

flowForeachExpr
    : FOREACH LEFT_SMALL_PAR constantExpr RIGHT_SMALL_PAR LEFT_BIG_PAR flowDeclaratorList RIGHT_BIG_PAR {
        SIMForeachExpression *expr = new SIMForeachExpression;
        expr->loopCnt = $3;
        expr->loopBlock = $6;
        $$ = expr;
    }
;

hardwareBlock
    : HARDWARE ASSIGN LEFT_MID_PAR hardwareDeclaratorList RIGHT_MID_PAR SEMI {
        $$ = $4;
    }
;

operatorBlock
    : OPERATOR ASSIGN LEFT_MID_PAR operatorDeclaratorList RIGHT_MID_PAR SEMI {
        $$ = $4;
    }
;

hardwareDeclaratorList
    : hardwareDeclarator {
        SIMHardwareBlock *block = new SIMHardwareBlock;
        block->exprs.emplace_back($1);
        $$ = block;
    }
    | hardwareDeclaratorList hardwareDeclarator {
        SIMHardwareBlock *block = dynamic_cast<SIMHardwareBlock *>($1);
        block->exprs.emplace_back($2);
        $$ = block;
    }
;

operatorDeclaratorList
    : operatorDeclarator {
        SIMOperatorBlock *block = new SIMOperatorBlock;
        block->exprs.emplace_back($1);
        $$ = block;
    }
    | operatorDeclaratorList operatorDeclarator {
        SIMOperatorBlock *block = dynamic_cast<SIMOperatorBlock *>($1);
        block->exprs.emplace_back($2);
        $$ = block;
    }
;

hardwareDeclarator
    : varExpr LEFT_SMALL_PAR constantExpr RIGHT_SMALL_PAR COMMA {
        SIMHardwareExpr *expr = new SIMHardwareExpr;
        expr->hardwareName = $1;
        expr->hardwareCnt = $3;
        $$ = expr;
    }
;

operatorDeclarator
    : varExpr LEFT_SMALL_PAR varExpr COMMA constantExpr RIGHT_SMALL_PAR COMMA {
        SIMOperatorExpr *expr = new SIMOperatorExpr;
        expr->opName = $1;
        expr->hardwareName = $3;
        expr->time = $5;
        $$ = expr;
    }
;

varExpr
    : SYMBOL {
        SIMSymbol *symbol = new SIMSymbol;
        symbol->name = std::string(yylval.str);
        $$ = symbol;
    }
;

constantExpr
    : I_CONSTANT {
        $$ = yylval.iVal;
    }
;

%%
