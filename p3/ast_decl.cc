/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "errors.h"

extern SymbolTable *symbols;
Type *funcReturnType;
Type *classType;
        
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

Type* VarDecl::GetType() {
    return type;
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

void ClassDecl::AddChildren(Hashtable<FnDecl*> *func) { 
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            // Add VarDecl symbols to symbol table
            // Makes sure we don't redeclare variables in derived class
            if (dynamic_cast<VarDecl*>(members->Nth(i)))
                members->Nth(i)->AddSymbol(true);
            // Add type signiture to some other table
            else if (dynamic_cast<FnDecl*>(members->Nth(i)))
                members->Nth(i)->AddTypeSignitures(func);
        }
    }
}

void ClassDecl::Check() {
    if (extends) extends->Check(LookingForClass);
    if (implements) implements->CheckAll();
}

void ClassDecl::CheckChildren() {
    if (checked) return;
    classType = new NamedType(id);

    symbols->Push();
    extFun = new Hashtable<FnDecl*>();
    Hashtable<FnDecl*> *impFun = new Hashtable<FnDecl*>();
    Hashtable<FnDecl*> *memberFun = new Hashtable<FnDecl*>();
    if (implements) {
        InterfaceDecl *temp;
        for (int i = 0; i < implements->NumElements(); i++) {
            temp = implements->Nth(i)->GetInterface();
            if (temp) {
                temp->CheckChildren();
                temp->AddChildren(impFun);
            }
        }
    }
    if (extends) {
        ClassDecl *ex = extends->GetClass();
        if (ex) {
            ex->CheckChildren();
            ex->AddChildren(extFun);
        }
    }
    if (members) {
        members->AddSymbolAll(true);
        members->CheckAll();
        members->CheckChildrenAll();
        Decl *temp;
        FnDecl *tempFn;
        for (int i = 0; i < members->NumElements(); i++) {
            temp = members->Nth(i);
            tempFn = dynamic_cast<FnDecl*>(temp);
            if (tempFn)
            {
              //add the member function to the hashmap
              tempFn->AddTypeSignitures(memberFun);
              temp->CheckTypeSignitures(impFun);
              temp->CheckTypeSignitures(extFun);
            }
        }
    }
    //check for classes not fully implementing interface
    if (implements) {
        InterfaceDecl *temp;
        Hashtable<FnDecl*> *impFunCheck;
        for (int i = 0; i < implements->NumElements(); i++) {
            temp = implements->Nth(i)->GetInterface();
            if (temp) {
                  //check that each FnDecl in this interface im implemented
                  if (!temp->CoversFunctions(memberFun)) {
                      ReportError::InterfaceNotImplemented(this,implements->Nth(i));
                  }
                
            }
        }
    }
    
    delete extFun;
    delete impFun;
    delete memberFun;
    scope = symbols->Pop();
    checked = true;
    classType = NULL;
}


bool InterfaceDecl::CoversFunctions(Hashtable<FnDecl*> *classFunctions) {
    for (int i = 0; i < members->NumElements(); i++) {
        FnDecl* currFunction = dynamic_cast<FnDecl*>(members->Nth(i));
        if (currFunction) {
          FnDecl* classFn = classFunctions->Lookup(currFunction->GetName());
          if (classFn) {
            //test that the function signatures are equal
            return currFunction->EqualSignature(classFn);
          }else{
            return false;
          }
        }
    }
    return true;
}

Type* ClassDecl::GetType() {
    return new NamedType(id);
}

Decl* ClassDecl::CheckMember(Identifier *id) {
    if (scope) return scope->Lookup(id->GetName());
    return NULL;
}

// Check if this is convertable to other 
bool ClassDecl::ConvertableTo(Type *other) {
    if (extends) {
        if (extends->EqualType(other)) return true;
        if (extends->GetClass()->ConvertableTo(other)) return true;
    }
    if (implements) {
        for (int i = 0; i < implements->NumElements(); i++) {
            if (implements->Nth(i)->EqualType(other)) return true;
        }
    }
    return false;
}


InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::CheckChildren() {
    if (checked) return;
    
    symbols->Push();
    if (members) {
        members->AddSymbolAll(true);
        members->CheckAll();
    }
    symbols->Pop();
    checked = true;
}

void InterfaceDecl::AddChildren(Hashtable<FnDecl*> *func)
{
  if (members) {
    for (int i = 0; i < members->NumElements(); i++) {
        if (dynamic_cast<FnDecl*>(members->Nth(i)))
            members->Nth(i)->AddTypeSignitures(func);
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
        formals->AddSymbolAll(true);
        formals->CheckAll();
    }
    symbols->Pop();
}

Type* FnDecl::GetType() {
    return returnType;
}

/*
 * Add the function name and some way to represent it's type signiture
 */
void FnDecl::AddTypeSignitures(Hashtable<FnDecl*> *func) {
    func->Enter(this->id->GetName(), this, false);
}

bool FnDecl::EqualSignature(FnDecl* other) {
    // Check if type signitures match
    // Compare Return types and argument types
    Type *temp = other->returnType;
    if (!(this->returnType->EqualType(other->returnType))) {
        return false;
    }
    else if (formals->NumElements() != other->formals->NumElements()) {
        return false;
    }
    else {
        for (int i = 0; i < formals->NumElements(); i++) {
            if (!(formals->Nth(i)->GetType()->EqualType(other->formals->Nth(i)->GetType())))
            {
                return false;
            }
        }
    }
    return true;
}

void FnDecl::CheckTypeSignitures(Hashtable<FnDecl*> *func) {
    // Check if Base class declares function
    FnDecl *base = func->Lookup(this->id->GetName());
    if (base) {
        if (!EqualSignature(base)) {
          ReportError::OverrideMismatch(this);
        }
        func->Remove(this->id->GetName(), base);
    }
}

void FnDecl::CheckChildren() {
    funcReturnType = returnType;
    symbols->Push();
    if (formals) formals->AddSymbolAll(false);
    if (body) body->Check();
    symbols->Pop();
    funcReturnType = NULL;
}

