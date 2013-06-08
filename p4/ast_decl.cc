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
extern bool error;
ClassDecl *inClass = NULL;
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
    memberVars = new List<Decl*>;
    memberFuncs = new List<Decl*>;
    funcOffsets = new Hashtable<int>();
    emitted = false;
}

Type* ClassDecl::GetType() {
    return new NamedType(id);
}

void ClassDecl::AddSymbols() {
    symbols->Add(id->GetName(), this);
    scope = symbols->Push();
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            if (!dynamic_cast<VarDecl*>(members->Nth(i)))
                members->Nth(i)->AddSymbols();
        }
    }
    symbols->Pop();
}

Decl* ClassDecl::SearchMembers(char *name) {
    if (error) cout << "ClassDecl::SearchMembers" << endl;
    /*
    if (extends) {
        Decl *decl = symbols->Search(extends->GetName())->SearchMembers(name);
        if (decl) return decl;
    }   
    for (int i = 0; i < members->NumElements(); i++) {
        if (strcmp(members->Nth(i)->GetName(), name) == 0) 
            return members->Nth(i);
    }
    */
    for (int i = 0; i < memberVars->NumElements(); i++) {
        if (strcmp(memberVars->Nth(i)->GetName(), name) == 0) 
            return memberVars->Nth(i);
    }
    for (int i = 0; i < memberFuncs->NumElements(); i++) {
        if (strcmp(memberFuncs->Nth(i)->GetName(), name) == 0) 
            return memberFuncs->Nth(i);
    }
    return NULL;
}

Location* ClassDecl::Emit(CodeGenerator* codeGen) {
    if (emitted) return NULL;

    if (error) cout << "ClassDecl::Emit" << endl;
    SymbolTable *temp = symbols;
    symbols = scope;
    offset = CodeGenerator::OffsetToFirstParam;
    inClass = this;
    int fnOffset = 0;
    List<const char*> *functions = new List<const char*>();

    if (extends) {
        Decl *decl = symbols->Search(extends->GetName());
        // Add extends vars to the list of vars to search
        List<Decl*> *baseVars = decl->GetMemberVars();
        for (int i = 0; i < baseVars->NumElements(); i++) {
            memberVars->Append(baseVars->Nth(i));
            offset += CodeGenerator::VarSize;
        }

        // Add functions to the list 
        List<Decl*> *baseFunc = decl->GetMemberFunc();
        for (int i = 0; i < baseFunc->NumElements(); i++) {
            memberFuncs->Append(baseFunc->Nth(i));
            //functions->Append(baseFunc->Nth(i));
            fnOffset += CodeGenerator::VarSize;
        }
    }
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            Decl *decl = members->Nth(i);
            if (dynamic_cast<VarDecl*>(decl)) {
                decl->SetOffset(offset);
                offset += CodeGenerator::VarSize;
                memberVars->Append(decl);
            }
            else if (dynamic_cast<FnDecl*>(decl)) {
                char label[80];
                sprintf(label, "_%s.%s", id->GetName(), decl->GetName());
                decl->SetLabel((char*)label);

                for (int j = 0; j < memberFuncs->NumElements(); j++) {
                    if (strcmp(decl->GetName(), memberFuncs->Nth(j)->GetName()) == 0) {
                        memberFuncs->RemoveAt(j);
                    }
                }
                memberFuncs->Append(decl);

                decl->SetOffset(fnOffset);
                fnOffset += CodeGenerator::VarSize;

                //codeGen->GenLabel(label);
                //decl->Emit(codeGen);
            }
        }
    }
    // Build VTable and offsets
    fnOffset = 0;
    for (int i = 0; i < memberFuncs->NumElements(); i++) {
        functions->Append(memberFuncs->Nth(i)->GetLabel());
        funcOffsets->Enter(memberFuncs->Nth(i)->GetName(), fnOffset);
        fnOffset += CodeGenerator::VarSize;
    }
    if (members) {
        for (int i = 0; i < members->NumElements(); i++) {
            Decl *decl = members->Nth(i);
            if (dynamic_cast<FnDecl*>(decl)) {
                codeGen->GenLabel(decl->GetLabel());
                decl->Emit(codeGen);
            }
        }
    }

    codeGen->GenVTable(id->GetName(), functions);
    inClass = NULL;
    emitted = true;

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
    if (error) cout << id << endl;
    SymbolTable *temp = symbols;
    fn_offset = CodeGenerator::OffsetToFirstLocal;
    int offsetParam = CodeGenerator::OffsetToFirstParam;
    symbols = scope;
    //function = scope;
    function = this;

    VarDecl *thiss;
    if (inClass) {
        VarDecl *thiss = new VarDecl(new Identifier(*this->GetLocation(), "this"), inClass->GetType());
        formals->InsertAt(thiss, 0);
        symbols->Add((char*)"this", thiss);
    }

    //deal with formals
    int n = formals->NumElements();
    for (int i = 0; i < n; i++) {
        VarDecl* v = formals->Nth(i);
        v->SetLoc(offsetParam, true);
        offsetParam += CodeGenerator::VarSize;
    }

    if (body) {
        //fn_offset = CodeGenerator::OffsetToFirstLocal;
        if (!inClass) codeGen->GenLabel(id->GetName());
        BeginFunc *beginFunc = codeGen->GenBeginFunc();
        body->Emit(codeGen);
        beginFunc->SetFrameSize(CodeGenerator::OffsetToFirstLocal - fn_offset);
        codeGen->GenEndFunc();
    }
  
    symbols = temp;
    function = NULL;
    return NULL;
}


