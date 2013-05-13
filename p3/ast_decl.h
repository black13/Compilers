/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp3: You will need to extend the Decl classes to implement 
 * semantic processing including detection of declaration conflicts 
 * and managing scoping issues.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"
#include "hashtable.h"

class Type;
class NamedType;
class Identifier;
class Stmt;
class FnDecl;


class Decl : public Node 
{
  protected:
    Identifier *id;
    bool checked;
  
  public:
    Decl(Identifier *name);
    void AddSymbol(bool output) { if (id) id->AddSymbol(this, output); };
    bool IsChecked() { return checked; }
    virtual void Check() {};
    virtual void CheckChildren() {};
    virtual void AddChildren(Hashtable<FnDecl*> *) {};
    virtual void CheckTypeSignitures(Hashtable<FnDecl*> *) {};
    virtual void AddTypeSignitures(Hashtable<FnDecl*> *) {};
    friend ostream& operator<<(ostream& out, Decl *d) { return out << d->id; }
};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    
  public:
    VarDecl(Identifier *name, Type *type);
    void Check();
    Type * GetType();
};

class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;
    
  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    void Check();
    void CheckChildren();
    void AddTypeSignitures(Hashtable<FnDecl*> *);
    void CheckTypeSignitures(Hashtable<FnDecl*> *);
};

class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;

  public:
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    void AddChildren(Hashtable<FnDecl*> *);
    void Check();
    void CheckChildren();
};

class InterfaceDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
    void CheckChildren();
    void AddChildren(Hashtable<FnDecl*> *);
};



#endif
