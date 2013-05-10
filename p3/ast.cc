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

void Identifier::AddClass(Decl* parent) {
    classes->Add(name, parent);
}

void Identifier::AddInterface(Decl* parent) {
    interfaces->Add(name, parent);
}

void Identifier::CheckType(reasonT whyNeeded) {
    Decl *decl = symbols->Search(name);
    if (decl == NULL) ReportError::IdentifierNotDeclared(this, whyNeeded);
}

Decl* Identifier::GetDecl() {
    return symbols->Search(name);
}
