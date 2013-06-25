/* File: ast_type.h
 * ----------------
 * In our parse tree, Type nodes are used to represent and
 * store type information. The base Type class is used
 * for built-in types, the NamedType for classes and interfaces,
 * and the ArrayType for arrays of other types.  
 *
 * pp3: You will need to extend the Type classes to implement
 * the type system and rules for type equivalency and compatibility.
 */
 
#ifndef _H_ast_type
#define _H_ast_type

#include "ast.h"
#include "list.h"
#include "errors.h"
#include <iostream>
using namespace std;


class Type : public Node 
{
  protected:
    char *typeName;

  public :
    static Type *intType, *doubleType, *boolType, *voidType,
                *nullType, *stringType, *errorType;

    Type(yyltype loc) : Node(loc) {}
    Type(const char *str);
    
    virtual void PrintToStream(ostream& out) { out << typeName; }
    friend ostream& operator<<(ostream& out, Type *t) { t->PrintToStream(out); return out; }
    bool EqualType(Type *other);
    bool ConvertableTo(Type *other);
    virtual const char* GetName() { return typeName; }
    virtual void Check() {};
    virtual void Check(reasonT reason) {};
    virtual Type* CheckType(reasonT reason) { return this; };
    virtual ClassDecl* GetClass() { return NULL; };
    virtual Type* GetType() { return NULL; }; 
};

class NamedType : public Type 
{
  protected:
    Identifier *id;
    
  public:
    NamedType(Identifier *i);
    
    void PrintToStream(ostream& out) { out << id; }
    void Check(reasonT reason);
    void Check();
    Type* CheckType(reasonT reason) { return id->CheckType(reason); }; 
    const char* GetName() { return id->GetName(); }
    ClassDecl* GetClass();
    InterfaceDecl* GetInterface();
};

class ArrayType : public Type 
{
  protected:
    Type *elemType;

  public:
    ArrayType(yyltype loc, Type *elemType);
    ArrayType(Type *elemType);
    
    void PrintToStream(ostream& out) { out << elemType << "[]"; }
    void Check(reasonT reason);
    Type* CheckType(reasonT reason) { return elemType->CheckType(reason); }; 
    Type* GetType() { return elemType; }; 
    const char* GetName() { return elemType->GetName(); }
};

 
#endif
