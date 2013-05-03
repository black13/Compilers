/**
 * SymbolTable Class
 *  TODO
 */

#ifndef _H_symbol_table
#define _H_symbol_table

#include <list>
#include "utility.h"  // for Assert()
#include "location.h"
#include "hashtable.h"
#include "ast_decl.h"

class SymbolTable {

 private:
   std::list<Hashtable<Decl*>*> elems;

 public:
    // Create a new empty list
    SymbolTable() {}

    // Returns count of elements currently in list
    const int Size() const
    { 
      return (const int)elems.size(); 
    }

    // Adds element to list end
    // Call this whenever we go int 
    void Push()
    { 
      elems.push_back(new Hashtable<Decl*>()); 
    }

    // Removes head
    void Pop()
    { 
      elems.pop_back();
    }

    // Checks if id exists in current scope 
    Decl* SearchHead(char* id)
    { 
      return (elems.back()->Lookup(id));
    }

    // Find the loc in the nearest scope
    // if not found returns NULL
    Decl* Search(char* id)
    {
      for (std::list<Hashtable<Decl*>*>::reverse_iterator rit=elems.rbegin(); rit!=elems.rend(); ++rit)
      {
        Decl *decl = (*rit)->Lookup(id);
        if (decl != NULL)
        {
          return decl;
        }
      }
      return NULL;
    }

    // Add a new declared variable to current scope
    void Add(char* id, Decl* decl)
    {
      //TODO add redecleration checking
      elems.back()->Enter(id, decl, false);
    }
};

#endif

