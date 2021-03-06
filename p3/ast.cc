/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf
#include "list.h"
#include "errors.h"
#include "hashtable.h"

extern SymbolTable *symbols;

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}

Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

void Identifier::AddSymbol(Decl* parent, bool output) {
    Decl *decl = symbols->SearchHead(name);
    if (decl == NULL) symbols->Add(name, parent);
    else if (output) ReportError::DeclConflict(parent, decl);
}

Type* Identifier::CheckType(reasonT whyNeeded) {
    Decl *decl;
    if (whyNeeded == LookingForType) {
        decl = this->GetClass();
        if (!decl) decl = this->GetInterface();
    }
    else if (whyNeeded == LookingForClass) {
        decl = this->GetClass();
    }
    else if (whyNeeded == LookingForInterface) {
        decl = this->GetInterface();
    }
    else if (whyNeeded == LookingForVariable) {
        decl = this->GetVariable();
    }
    else if (whyNeeded == LookingForFunction) {
        decl = this->GetFunction();
    }
    else {
        decl = symbols->Search(name);
    }

    if (decl) {
        return decl->GetType();
    }
    
    ReportError::IdentifierNotDeclared(this, whyNeeded);
    return NULL;
}

const char * Identifier::GetName() {
  return this->name;
}

ClassDecl* Identifier::GetClass() {
    Decl* found = symbols->Search(name);
    ClassDecl* c = dynamic_cast<ClassDecl*>(found);
    return c;
}

InterfaceDecl* Identifier::GetInterface() {
    Decl* found = symbols->Search(name);
    InterfaceDecl* c = dynamic_cast<InterfaceDecl*>(found);
    return c;
}

VarDecl* Identifier::GetVariable() {
    Decl* found = symbols->Search(name);
    VarDecl* c = dynamic_cast<VarDecl*>(found);
    return c;
}

FnDecl* Identifier::GetFunction() {
    Decl* found = symbols->Search(name);
    FnDecl* c = dynamic_cast<FnDecl*>(found);
    return c;
}
