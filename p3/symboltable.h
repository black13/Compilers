/**
 * SymbolTable Class
 *  TODO
 */

#ifndef _H_symbol_table
#define _H_symbol_table

#include <list>
#include "utility.h"  // for Assert()
#include "hashtable.h"

class SymbolTable {

 private:
   std::list<Hashtable<int>*> elems;

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
      elems.push_back(new Hashtable<int>()); 
    }

    // Removes head
    void Pop()
    { 
      elems.pop_back();
    }

    // Checks if id exists in current scope 
    bool InHead(char* id)
    { 
      return (elems.back()->Lookup(id) != NULL);
    }

    // Find the loc in the nearest scope
    // if not found returns NULL
    int Search(char* id)
    {
      for (std::list<Hashtable<int>*>::reverse_iterator rit=elems.rbegin(); rit!=elems.rend(); ++rit)
      {
        int location = (*rit)->Lookup(id);
        if (location != NULL)
        {
          return location;
        }
      }
      return NULL;
    }

    // Add a new declared variable to current scope
    void Add(char* id, int loc)
    {
      //TODO add redecleration checking
      elems.back()->Enter(id, loc, false);
    }
};

#endif

