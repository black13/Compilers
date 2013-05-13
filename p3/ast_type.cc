/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */

#include <string.h>
#include "ast_type.h"
#include "ast_decl.h"
 
/* Class constants
 * ---------------
 * These are public constants for the built-in base types (int, double, etc.)
 * They can be accessed with the syntax Type::intType. This allows you to
 * directly access them and share the built-in types where needed rather that
 * creates lots of copies.
 */

Type *Type::intType    = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType   = new Type("void");
Type *Type::boolType   = new Type("bool");
Type *Type::nullType   = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType  = new Type("error"); 

Type::Type(const char *n) {
    Assert(n);
    typeName = strdup(n);
}

bool Type::EqualType(Type *other) {
    int x = strcmp(typeName, other->GetName());    
    return x == 0;    
}

	
NamedType::NamedType(Identifier *i) : Type(*i->GetLocation()) {
    Assert(i != NULL);
    (id=i)->SetParent(this);
} 

void NamedType::Check(reasonT reason) {
    if (id) id->CheckType(reason);
}

void NamedType::Check() {
    if (id) id->CheckType(LookingForInterface);
}

ClassDecl* NamedType::GetClass() {
    return id->GetClass();
}

InterfaceDecl* NamedType::GetInterface() {
    return id->GetInterface();
}

ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}

void ArrayType::Check(reasonT reason) {
    if (elemType) elemType->Check(reason);
}


