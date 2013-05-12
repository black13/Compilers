/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "errors.h"
#include "symboltable.h"

extern SymbolTable *symbols;
extern Hashtable<FnDecl*> *functions;
        
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void VarDecl::Check() {
    if (type) type->Check(LookingForType);
}

Type * VarDecl::GetType() {
    return this->type;
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

void ClassDecl::AddChildren() { 
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            // Add VarDecl symbols to symbol table
            // Makes sure we don't redeclare variables in derived class
            if (dynamic_cast<VarDecl*>(members->Nth(i)))
                members->Nth(i)->AddSymbol();
            // Add type signiture to some other table
            else if (dynamic_cast<FnDecl*>(members->Nth(i)))
                members->Nth(i)->AddTypeSignitures();
        }
    }
}

void ClassDecl::Check() {
    if (extends) extends->Check(LookingForClass);
    if (implements) implements->CheckAll();
}

void ClassDecl::CheckChildren() {
    symbols->Push();
    functions = new Hashtable<FnDecl*>();
    if (implements)
    {
      for (int i = 0; i < implements->NumElements(); i++ )
      {
        //implements->Nth(i)->GetInterface()->AddFunctions();;
      }
    }
    if (extends) {
        ClassDecl *ex = extends->GetClass();
        if (ex) ex->AddChildren();
    }
    if (members) {
        members->AddSymbolAll();
        members->CheckAll();
        members->CheckTypeSignituresAll();
    }
    delete functions;
    symbols->Pop();
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::CheckChildren() {
    symbols->Push();
    if (members) {
        members->AddSymbolAll();
        members->CheckAll();
    }
    symbols->Pop();
}

void InterfaceDecl::AddFunctions()
{
  if (members)
  {
    for (int i = 0; i < members->NumElements(); i++)
    {
      FnDecl* f = dynamic_cast<FnDecl*>(members->Nth(i));
      if (f)
      {
        f->AddTypeSignitures();
      }
    }
  }
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

void FnDecl::Check() {
    symbols->Push();
    if (formals) {
        formals->AddSymbolAll();
        formals->CheckAll();
    }
}

/*
 * Add the function name and some way to represent it's type signiture
 */
void FnDecl::AddTypeSignitures() {
    functions->Enter(this->id->GetName(), this, false);
}

void FnDecl::CheckTypeSignitures() {
    // Check if Base class declares function
    FnDecl *base = functions->Lookup(this->id->GetName());
    if (base)
    {
        // Check if type signitures match
        // Compare Return types and argument types
        if (!base->returnType->IsEquivalentTo(this->returnType) ||
            formals->NumElements() != base->formals->NumElements())
        {
            ReportError::OverrideMismatch(this);
            return;
        }
        else
        {
          for (int i = 0; i < formals->NumElements(); i++)
          {
              if (!formals->Nth(i)->GetType()->IsEquivalentTo(base->formals->Nth(i)->GetType()))
              {
                //ReportError::OverrideMismatch(this);
                return;
              }
          }
        }
    }
}

void FnDecl::CheckChildren() {
    if (body) body->Check();
    symbols->Pop();
}

