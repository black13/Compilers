/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"

extern SymbolTable *symbols;
SymbolTable *global;

Program::Program(List<Decl*> *d) {
    codeGen = new CodeGenerator;
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    /* You can use your pp3 semantic analysis or leave it out if
     * you want to avoid the clutter.  We won't test pp4 against 
     * semantically-invalid programs.
     */
}
void Program::Emit() {
    /* pp4: here is where the code generation is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, generating instructions as you go.
     *      Each node can have its own way of translating itself,
     *      which makes for a great use of inheritance and
     *      polymorphism in the node classes.
     */
    
    //offset = CodeGenerator::OffsetToFirstGlobal;
    int n = decls->NumElements();

    global = symbols->Push();
    for (int i = 0; i < n; ++i) {
        decls->Nth(i)->AddSymbols();
    }

    // Emit all classes
    for (int i = 0; i < n; ++i) {
        if (dynamic_cast<ClassDecl*>(decls->Nth(i)))
            decls->Nth(i)->Emit(codeGen);
    }

    // Emit all decls
    for (int i = 0; i < n; ++i) {
        decls->Nth(i)->Emit(codeGen);
    }

    //final step
    codeGen->DoFinalCodeGen();
    symbols->Pop();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::AddSymbols() {
    scope = symbols->Push();
    for (int i = 0; i < decls->NumElements(); ++i) {
        decls->Nth(i)->AddSymbols();
    }
    for (int i = 0; i < stmts->NumElements(); ++i) {
        stmts->Nth(i)->AddSymbols();
    }
    symbols->Pop();
}

Location* StmtBlock::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    symbols = scope;

    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Emit(codeGen);
    }

    for (int i = 0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Emit(codeGen);
    }

    symbols = temp;
    return NULL;
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}


ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void ForStmt::AddSymbols() {
    scope = symbols->Push();
    if (body) body->AddSymbols();
    symbols->Pop();
}


Location* ForStmt::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    symbols = scope;

    char *label0 = codeGen->NewLabel();
    char *label1 = codeGen->NewLabel();
    breakLabel = label1;

    init->Emit(codeGen);
    codeGen->GenLabel(label0);
    codeGen->GenIfZ(test->Emit(codeGen), label1);
    body->Emit(codeGen);
    step->Emit(codeGen);
    codeGen->GenGoto(label0);
    codeGen->GenLabel(label1);

    symbols = temp;
    return NULL;
}

void WhileStmt::AddSymbols() {
    scope = symbols->Push();
    if (body) body->AddSymbols();
    symbols->Pop();
}

Location* WhileStmt::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    symbols = scope;

    char *label0 = codeGen->NewLabel();
    char *label1 = codeGen->NewLabel();
    breakLabel = label1;

    codeGen->GenLabel(label0);
    codeGen->GenIfZ(test->Emit(codeGen), label1);
    body->Emit(codeGen);
    codeGen->GenGoto(label0);
    codeGen->GenLabel(label1);

    symbols = temp;
    return NULL;
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::AddSymbols() {
    scope = symbols->Push();
    if (body) body->AddSymbols();
    if (elseBody) elseBody->AddSymbols();
    symbols->Pop();
}

Location* IfStmt::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    symbols = scope;

    char *label0 = codeGen->NewLabel();
    char *label1; 
    if (elseBody) label1 = codeGen->NewLabel();
    
    codeGen->GenIfZ(test->Emit(codeGen), label0);
    body->Emit(codeGen);

    if (elseBody) codeGen->GenGoto(label1);
    codeGen->GenLabel(label0);
    if (elseBody) {
        elseBody->Emit(codeGen);
        codeGen->GenLabel(label1);
    }

    symbols = temp;
    return NULL;
}

Location* BreakStmt::Emit(CodeGenerator* codeGen) {
    Node *parent = this->GetParent();
    while (parent && !parent->IsLoop()) parent = parent->GetParent();
    if (parent && parent->GetBreakLabel()) codeGen->GenGoto(parent->GetBreakLabel());
    return NULL;
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}


Location* ReturnStmt::Emit(CodeGenerator* codeGen) {
    if (expr) codeGen->GenReturn(expr->Emit(codeGen));
    else codeGen->GenReturn();

    return NULL;
}

PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

Location* PrintStmt::Emit(CodeGenerator* codeGen) {
  int n = args->NumElements();
  for (int i = 0; i<n; i++) {
    Expr* e = args->Nth(i);
    Type* t = e->GetType();

    //determine which print TAC to use
    BuiltIn b = NumBuiltIns;
    if (t->IsEquivalentTo(Type::intType))
      b = PrintInt;
    else if (t->IsEquivalentTo(Type::stringType))
      b = PrintString;
    else if (t->IsEquivalentTo(Type::boolType))
      b = PrintBool;
    
    //generate the code!
    codeGen->GenBuiltInCall(b,e->Emit(codeGen));
  }

  return NULL;
}

