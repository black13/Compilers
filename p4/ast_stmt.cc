/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


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
    
    int offset = CodeGenerator::OffsetToFirstGlobal;
    int n = decls->NumElements();

    //set locations for all VarDecls
    for (int i = 0; i < n; ++i) {
        VarDecl *d = dynamic_cast<VarDecl*>(decls->Nth(i));
        if (d) {
          d->SetLoc(offset);
          offset += d->GetBytes();
        }
    }

    for (int i = 0; i < n; ++i) {
        Decl* d = decls->Nth(i);
        d->Emit(codeGen);
    }

    //final step
    codeGen->DoFinalCodeGen();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

int StmtBlock::GetBytes() {
  int bytes = 0;
  int n = decls->NumElements();

  for (int i = 0; i < n; i++){
    bytes += decls->Nth(i)->GetBytes();
  }
  
  n = stmts->NumElements();
  for (int i = 0; i < n; i++){
    bytes += stmts->Nth(i)->GetBytes();
  }
  return bytes;
}

Location* StmtBlock::Emit(CodeGenerator* codeGen) {
  int n = decls->NumElements();
  for (int i=0; i<n; i++) {
    VarDecl *v = dynamic_cast<VarDecl*>(decls->Nth(i));
    if (v) {
      //d->SetLoc(); //TODO
    }
  }
  n = stmts->NumElements();
  for (int i=0; i<n; i++) {
    stmts->Nth(i)->Emit(codeGen);
  }
  return NULL;
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

int ConditionalStmt::GetBytes() {
  int offset = test->GetBytes();
  offset += body->GetBytes();
  return offset;
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

int ForStmt::GetBytes(){
  int offset = LoopStmt::GetBytes();
  offset += init->GetBytes();
  offset += step->GetBytes();
  return offset;
}

Location* ForStmt::Emit(CodeGenerator* codeGen) {
  cout << "EMIT:TODO" << endl;
  return NULL;
}

Location* WhileStmt::Emit(CodeGenerator* codeGen) {
  cout << "EMIT:TODO" << endl;
  return NULL;
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

Location* IfStmt::Emit(CodeGenerator* codeGen) {
  cout << "EMIT:TODO" << endl;
  return NULL;
}

int IfStmt::GetBytes() {
  int offset = ConditionalStmt::GetBytes();
  offset += elseBody->GetBytes();
  return offset;
}

Location* BreakStmt::Emit(CodeGenerator* codegen) {
  cout << "EMIT:TODO" << endl;
  return NULL;
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

int ReturnStmt::GetBytes() {
  if (expr){
    return expr->GetBytes();
  }
  return 0;
}

Location* ReturnStmt::Emit(CodeGenerator* codeGen) {
  cout << "EMIT:TODO" << endl;
  return NULL;
}

PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

int PrintStmt::GetBytes() {
  int bytes = 0;
  int n = args->NumElements();
  for (int i = 0; i<n; i++) {
    bytes += args->Nth(i)->GetBytes();
  }
  return bytes;
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


