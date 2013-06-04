/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"

extern SymbolTable *symbols;
int fn_offset;
bool inClass = false;
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

Decl * Decl::SearchScope(char * name) {
    if (scope) return scope->Search(name);
    return NULL;
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

Location* VarDecl::Emit(CodeGenerator* codeGen) {
    return NULL;
}

void VarDecl::AddSymbols() {
    symbols->Add(id->GetName(), this);
}

void VarDecl::SetLoc(int location, bool func) {
    if (func) 
        this->loc = new Location(fpRelative, location, this->GetName());
    else 
        this->loc = new Location(gpRelative, location, this->GetName());
    
    //symbols->Add(id->GetName(), this);
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

void ClassDecl::AddSymbols() {
    symbols->Add(id->GetName(), this);
    scope = symbols->Push();
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            members->Nth(i)->AddSymbols();
        }
    }
    symbols->Pop();
}

Location* ClassDecl::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    symbols = scope;
    //symbols->Add(id->GetName(), this);
    offset = CodeGenerator::OffsetToFirstParam;
    List<const char*> *functions = new List<const char*>();
    inClass = true;
    int varOffset = 0;
    int fnOffset = 0;
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            Decl *decl = members->Nth(i);
            if (dynamic_cast<VarDecl*>(decl)) {
                decl->SetLoc(offset, true);
                decl->SetOffset(varOffset);
                varOffset += CodeGenerator::VarSize;
                offset += decl->GetBytes();
            }
            else if (dynamic_cast<FnDecl*>(decl)) {
                char label[80];
                sprintf(label, "_%s.%s", id->GetName(), decl->GetName());
                functions->Append(strdup(label));           

                // Generate This pointer
                //new Location(fpRelative, CodeGenerator::OffsetToFirstParam, "this");
                //symbols->Add("this", this);

                decl->SetOffset(fnOffset);
                fnOffset += CodeGenerator::VarSize;

                codeGen->GenLabel(label);
                decl->Emit(codeGen);
            }
        }
    }
    codeGen->GenVTable(id->GetName(), functions);
    inClass = false;

    symbols = temp;
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

void FnDecl::AddSymbols() {
    symbols->Add(id->GetName(), this);
    scope = symbols->Push();
    for (int i = 0; i < formals->NumElements(); i++) {
        formals->Nth(i)->AddSymbols();
    }
    if (body) body->AddSymbols();
    symbols->Pop();
}

Location* FnDecl::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    symbols = scope;
    int offset = CodeGenerator::OffsetToFirstParam;
    if (inClass) {
        new Location(fpRelative, offset, "this");
        symbols->Add("this", this);
        offset += CodeGenerator::VarSize;
    }

    //deal with formals
    int n = formals->NumElements();
    for (int i = 0; i < n; i++) {
        VarDecl* v = formals->Nth(i);
        v->SetLoc(offset, true);
        offset += v->GetBytes();
    }

    if (body) {
        fn_offset = CodeGenerator::OffsetToFirstLocal;
        if (!inClass) codeGen->GenLabel(id->GetName());
        codeGen->GenBeginFunc()->SetFrameSize(body->GetBytes());
        body->Emit(codeGen);
        codeGen->GenEndFunc();
    }
  
    symbols = temp;
    return NULL;
}


