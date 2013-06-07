/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"

extern SymbolTable *symbols;
int gp_offset = CodeGenerator::OffsetToFirstGlobal;
int fn_offset;
//Type *inClass = NULL;
ClassDecl *inClass = NULL;
//SymbolTable *function = NULL;
FnDecl *function = NULL;

Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

Decl * Decl::SearchScope(char * name) {
    if (scope) {
        return scope->Search(name);
    }
    return NULL;
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

VarDecl::VarDecl(const char *n, Type *t) {
    Assert(n != NULL && t != NULL);
    (id=new Identifier(n))->SetParent(this); 
    (type=t)->SetParent(this);
}


Location* VarDecl::Emit(CodeGenerator* codeGen) {
    if (function) {
        // TODO: switch these to eliminate changes in codegen.h
        //this->loc = codeGen->GenTempVar();
        this->loc = new Location(fpRelative, fn_offset, this->GetName());
        fn_offset -= CodeGenerator::VarSize;
        //codeGen->IncOffset();
    }
    else {
        this->loc = new Location(gpRelative, gp_offset, this->GetName());
        gp_offset += CodeGenerator::VarSize; 
    }
    return NULL;
}

void VarDecl::AddSymbols() {
    symbols->Add(id->GetName(), this);
}

void VarDecl::SetLoc(int location, bool func) {
    //cout << id << ": " << location << endl;
    if (func) 
        this->loc = new Location(fpRelative, location, this->GetName());
    else 
        this->loc = new Location(gpRelative, location, this->GetName());
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

Type* ClassDecl::GetType() {
    return new NamedType(id);
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

Decl* ClassDecl::SearchMembers(char *name) {
    for (int i = 0; i < members->NumElements(); i++) {
        if (strcmp(members->Nth(i)->GetName(), name) == 0) 
            return members->Nth(i);
    }
    return NULL;
}

Location* ClassDecl::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    symbols = scope;
    //symbols->Add(id->GetName(), this);
    offset = CodeGenerator::OffsetToFirstParam;
    List<const char*> *functions = new List<const char*>();
    //inClass = new NamedType(id);
    inClass = this;
    int varOffset = CodeGenerator::VarSize;
    int fnOffset = 0;
    list<Decl*> *memberVars;
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            Decl *decl = members->Nth(i);
            if (dynamic_cast<VarDecl*>(decl)) {
                decl->SetOffset(varOffset);
                varOffset += CodeGenerator::VarSize;
                //decl->SetLoc(offset, true);
                //offset += decl->GetBytes();
            }
            else if (dynamic_cast<FnDecl*>(decl)) {
                char label[80];
                sprintf(label, "_%s.%s", id->GetName(), decl->GetName());
                functions->Append(strdup(label));           

                decl->SetOffset(fnOffset);
                fnOffset += CodeGenerator::VarSize;

                codeGen->GenLabel(label);
                decl->Emit(codeGen);
            }
        }
    }
    codeGen->GenVTable(id->GetName(), functions);
    inClass = NULL;

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

Decl* FnDecl::SearchFormals(char *name) {
    for (int i = 0; i < formals->NumElements(); i++) {
        if (strcmp(formals->Nth(i)->GetName(), name) == 0) 
            return formals->Nth(i);
    }
    return NULL;
}

Location* FnDecl::Emit(CodeGenerator* codeGen) {
    SymbolTable *temp = symbols;
    fn_offset = CodeGenerator::OffsetToFirstLocal;
    int offset = CodeGenerator::OffsetToFirstParam;
    symbols = scope;
    //function = scope;
    function = this;
    
    if (inClass) {
        VarDecl *thiss = new VarDecl("this", inClass->GetType());
        // Set the location of "this" to fp+4 
        thiss->SetLoc(offset, true);
        formals->InsertAt(thiss, 0);
        symbols->Add((char*)"this", thiss);
        offset += CodeGenerator::VarSize;
    }

    //deal with formals
    int n = formals->NumElements();
    for (int i = 0; i < n; i++) {
        VarDecl* v = formals->Nth(i);
        v->SetLoc(offset, true);
        offset += CodeGenerator::VarSize;
    }

    if (body) {
        //fn_offset = CodeGenerator::OffsetToFirstLocal;
        if (!inClass) codeGen->GenLabel(id->GetName());
        //codeGen->GenBeginFunc()->SetFrameSize(body->GetBytes());
        BeginFunc *beginFunc = codeGen->GenBeginFunc();
        body->Emit(codeGen);
        beginFunc->SetFrameSize(CodeGenerator::OffsetToFirstLocal - fn_offset);
        codeGen->GenEndFunc();
    }
  
    symbols = temp;
    function = NULL;
    return NULL;
}


