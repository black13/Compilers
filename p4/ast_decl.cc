/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"

extern SymbolTable *symbols;
int fn_offset;
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

Location* VarDecl::Emit(CodeGenerator* codeGen) {
  return NULL;
}

void VarDecl::SetLoc(int location) {
    this->loc = new Location(gpRelative, location, this->GetName());
    symbols->Add(id->GetName(), this);
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

Location* ClassDecl::Emit(CodeGenerator* codeGen) {
  cout << "Class:TODO" << endl;
  return NULL;
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

Location* InterfaceDecl::Emit(CodeGenerator* codeGen) {
  cout << "Interface:TODO" << endl;
  return NULL;
}

	
FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

Location* FnDecl::Emit(CodeGenerator* codeGen) {
    int offset = CodeGenerator::OffsetToFirstParam;
    symbols->Add(id->GetName(), this);

    //deal with formals
    int n = formals->NumElements();
    for (int i = 0; i < n; i++) {
        VarDecl* v = formals->Nth(i);
        v->SetLoc(offset);
        offset += v->GetBytes();
    }

    if (body) {
        fn_offset = CodeGenerator::OffsetToFirstLocal;
        codeGen->GenLabel(id->GetName());
        codeGen->GenBeginFunc()->SetFrameSize(body->GetBytes());
        body->Emit(codeGen);
        codeGen->GenEndFunc();
    }
  
    return NULL;
}


