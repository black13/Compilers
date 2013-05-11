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
extern Hashtable<ClassDecl*> *classes;
extern Hashtable<InterfaceDecl*> *interfaces;

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

void Identifier::AddClass(ClassDecl* parent) {
    classes->Enter(name, parent, false);
}

void Identifier::AddInterface(InterfaceDecl* parent) {
    interfaces->Enter(name, parent, false);
}

void Identifier::CheckType(reasonT whyNeeded) {
    Decl *decl = symbols->Search(name);
    if (decl == NULL) ReportError::IdentifierNotDeclared(this, whyNeeded);
}

Decl* Identifier::GetDecl() {
    return symbols->Search(name);
}
