/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf
#include "symboltable.h"
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

void Identifier::AddSymbol(Decl* parent) {
    Decl *decl = symbols->SearchHead(name);
    if (decl == NULL) symbols->Add(name, parent);
    else ReportError::DeclConflict(parent, decl);
}

void Identifier::CheckType(reasonT whyNeeded) {
    Decl *decl = symbols->Search(name);
    if (decl == NULL) ReportError::IdentifierNotDeclared(this, whyNeeded);
}

const char * Identifier::GetName() {
  return this->name;
}

/*
 * return the Cecl of the class, if not found returns null
 */
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
