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

void Identifier::CheckSymbol(Node* parent) {
    Decl *decl = symbols->SearchHead(name);
    if (decl == NULL) symbols->Add(name, (Decl*)parent);
    else ReportError::DeclConflict((Decl*)parent, decl);
}

