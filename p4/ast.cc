/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf

extern SymbolTable *symbols;
bool error = false;

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

Identifier::Identifier(const char *n) {
    name = strdup(n);
} 


Type * Identifier::GetType() {
    Decl *decl = symbols->Search(name);
    if (decl) return decl->GetType();
    return NULL;
}

/*
void Identifier::AddSymbol(Decl* parent) {
    Decl *decl = symbols->SearchHead(name);
    if (decl == NULL) symbols->Add(name, parent);
}
*/

