/**
 * File: ast.h
 * ----------- 
 * This file defines the abstract base class Node and the concrete 
 * Identifier and Error node subclasses that are used through the tree as 
 * leaf nodes. A parse tree is a hierarchical collection of ast nodes (or, 
 * more correctly, of instances of concrete subclassses such as VarDecl,
 * ForStmt, and AssignExpr).
 * 
 * Location: Each node maintains its lexical location (line and columns in 
 * file), that location can be NULL for those nodes that don't care/use 
 * locations. The location is typcially set by the node constructor.  The 
 * location is used to provide the context when reporting semantic errors.
 *
 * Parent: Each node has a pointer to its parent. For a Program node, the 
 * parent is NULL, for all other nodes it is the pointer to the node one level
 * up in the parse tree.  The parent is not set in the constructor (during a 
 * bottom-up parse we don't know the parent at the time of construction) but 
 * instead we wait until assigning the children into the parent node and then 
 * set up links in both directions. The parent link is typically not used 
 * during parsing, but is more important in later phases.
 *
 * Semantic analysis: For pp3 you are adding "Check" behavior to the ast
 * node classes. Your semantic analyzer should do an inorder walk on the
 * parse tree, and when visiting each node, verify the particular
 * semantic rules that apply to that construct.

 */

#ifndef _H_ast
#define _H_ast

#include <stdlib.h>   // for NULL
#include "location.h"
#include "errors.h"
#include <iostream>
using namespace std;

class Decl;
class ClassDecl;
class InterfaceDecl;
class VarDecl;
class FnDecl;

class Node  {
  protected:
    yyltype *location;
    Node *parent;

  public:
    Node(yyltype loc);
    Node();
    virtual ~Node() {}
    
    yyltype *GetLocation()   { return location; }
    void SetParent(Node *p)  { parent = p; }
    Node *GetParent()        { return parent; }
    virtual void Check() {};
    void Check(reasonT) {};

};
   

class Identifier : public Node 
{
  protected:
    char *name;
    
  public:
    Identifier(yyltype loc, const char *name);
    void AddSymbol(Decl* parent, bool output);
    void AddClass(ClassDecl* parent);
    void AddInterface(InterfaceDecl* parent);
    Type* CheckType(reasonT whyNeeded);
    const char * GetName();
    friend ostream& operator<<(ostream& out, Identifier *id) { return out << id->name; }
    ClassDecl* GetClass();
    InterfaceDecl* GetInterface();
    VarDecl* GetVariable();
    FnDecl* GetFunction();
};


// This node class is designed to represent a portion of the tree that 
// encountered syntax errors during parsing. The partial completed tree
// is discarded along with the states being popped, and an instance of
// the Error class can stand in as the placeholder in the parse tree
// when your parser can continue after an error.
class Error : public Node
{
  public:
    Error() : Node() {}
};

#endif

/**
 * SymbolTable Class
 *  
 */

#ifndef _H_symbol_table
#define _H_symbol_table

#include <list>
#include "utility.h"  // for Assert()
#include "location.h"
#include "hashtable.h"
#include "ast_decl.h"

class Table {
  public:
    Hashtable<Decl*> *table;
    Table *parent;
    Table(Table *p) { table = new Hashtable<Decl*>(); parent = p; };
    friend ostream& operator<<(ostream& out, Table *tbl) { 
      Iterator<Decl*> it = tbl->table->GetIterator();
      Decl* temp = it.GetNextValue();
      while (temp){
        out << temp << " ";
        temp = it.GetNextValue();
      }
      return out;
    }
};

class SymbolTable {

 private:
   Table *branch;
   int level;

 public:
    SymbolTable() { branch = NULL; level = 0; }
    SymbolTable(Table *table, int lvl) { branch = table; level = lvl; }

    // Returns count of elements currently in list
    const int Size() const
    { 
      return level;
    }

    // Adds element to list end
    // Call this whenever we go int 
    SymbolTable* Push()
    { 
      branch = new Table(branch);
      level++;
      return new SymbolTable(branch, level);
    }

    // Removes head
    void Pop()
    { 
      if (branch) {
        branch = branch->parent;
        level--;
      }
    }

    // Checks if id exists in current scope 
    Decl* SearchHead(char* id)
    { 
      if (branch && branch->table) return branch->table->Lookup(id);
      return NULL;
    }

    // Find the loc in the nearest scope
    // if not found returns NULL
    Decl* Search(char* id) {
        cout << this << endl;
        //cout << "0" << endl;
        if (branch != NULL) {
            //cout << "1" << endl;
            //cout << branch << endl;
            Table *temp = branch;
            do {
                //cout << "2" << endl;
                Decl *decl = temp->table->Lookup(id);
                if (decl != NULL)
                {
                    return decl;
                }
                temp = temp->parent;
            } while (temp != NULL);
        }
        return NULL;
    }

    // Add a new declared variable to current scope
    void Add(char* id, Decl* decl)
    {
      if (branch && branch->table) branch->table->Enter(id, decl, false);
    }

    friend ostream& operator<<(ostream& out, SymbolTable *sym) {
      if (sym->branch) {
        out << "Level: " << sym->level;
        Table *temp = sym->branch;
        while (temp != NULL) {
          out << "[" << temp << "]";
          temp = temp->parent;
        }
      } else {
        out << "Empty";
      }
      return out;
    }
};

#endif

