/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "list.h"

extern SymbolTable *symbols;
extern Type *funcReturnType;
bool inLoop;

Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */
    symbols->Push();
    if (decls) {
        decls->AddSymbolAll(true);
        decls->CheckAll();
        decls->CheckChildrenAll();
    }
    symbols->Pop();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::Check() {
    symbols->Push();
    if (decls) {
        decls->AddSymbolAll(true);
        decls->CheckAll();
    }
    if (stmts) {
        stmts->CheckAll();
    }
    symbols->Pop();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

void ConditionalStmt::Check() {
    if (body) body->Check();
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void ForStmt::Check() {
    if (test) {
        Type *type = test->CheckType();
        if (type) { 
            if (!type->EqualType(Type::boolType)) 
                ReportError::TestNotBoolean(test);
        }
    }
    if (init) init->Check();
    if (step) step->Check();
    inLoop = true;
    if (body) body->Check();
    inLoop = false;
}

void WhileStmt::Check() {
    if (test) {
        Type *type = test->CheckType();
        if (type) { 
            if (!type->EqualType(Type::boolType)) 
                ReportError::TestNotBoolean(test);
        }
    }
    inLoop = true;
    if (body) body->Check();
    inLoop = false;
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::Check() {
    if (test) {
        Type *type = test->CheckType();
        if (!type || !type->EqualType(Type::boolType)) 
            ReportError::TestNotBoolean(test);
    }
    if (body) body->Check();
    if (elseBody) elseBody->Check();
}


void BreakStmt::Check() {
    if (!inLoop) ReportError::BreakOutsideLoop(this);
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}
  
void ReturnStmt::Check() {
    Type *given = expr->CheckType();
    if (given && !funcReturnType->ConvertableTo(given))
        ReportError::ReturnMismatch(this, given, funcReturnType);
}

PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::Check() {
    for (int i = 0; i < args->NumElements(); i++) {
        Type *type = args->Nth(i)->CheckType();
        if (type && !(type->EqualType(Type::intType) || type->EqualType(Type::boolType) || type->EqualType(Type::stringType)))
            ReportError::PrintArgMismatch(args->Nth(i), i+1, type);
    }
}


