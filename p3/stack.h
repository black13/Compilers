/**
 * File: list.h
 * ------------
 * Simple list class for storing a linear collection of elements. It
 * supports operations similar in name to the CS107 CVector -- nth, insert,
 * append, remove, etc.  This class is nothing more than a very thin
 * cover of a STL deque, with some added range-checking. Given not everyone
 * is familiar with the C++ templates, this class provides a more familiar
 * interface.
 *
 * It can handle elements of any type, the typename for a List includes the
 * element type in angle brackets, e.g.  to store elements of type double,
 * you would use the type name List<double>, to store elements of type
 * Decl *, it woud be List<Decl*> and so on.
 *
 * Here is some sample code illustrating the usage of a List of integers
 *
 *   int Sum(List<int> *list) {
 *       int sum = 0;
 *       for (int i = 0; i < list->NumElements(); i++) {
 *          int val = list->Nth(i);
 *          sum += val;
 *       }
 *       return sum;
 *    }
 */

#ifndef _H_list
#define _H_list

#include <list>
#include "utility.h"  // for Assert()
#include "hashtable.h"
using namespace std;

class Node;

template<class Element> class Stack {

 private:
    list<HashTable> elems;

 public:
    // Create a new empty list
    Stack() {}

    // Returns count of elements currently in list
    int NumElements() const
    { 
      return elems.size(); 
    }

    // Adds element to list end
    void Append()
    { 
      elems.push_back(new HashTable<int>()); 
    }

    // Removes head
    void Remove()
    { 
      elems.pop_back();
    }

    // Checks if id exists in current scope 
    bool InHead(char* id)
    { 
      if (elems.back().Lookup(id) != NULL) return true; 
      return false;
    }

    // Find the loc in the nearest scope
    // if not found returns NULL
    int Search(char* id)
    {
      for (std::list<HashTable>::reverse_iterator rit=elems.rbegin(); rit!=elems.rend(); ++rit)
      {
        int location = rit->Lookup(id);
        if (location != NULL)
        {
          return location;
        }
      }
      return NULL;
    }

    // Add a new declared variable to current scope
    void Push(char* id, int loc)
    {
      elems.back().Enter(id, loc, false);
    }
          
       // These are some specific methods useful for lists of ast nodes
       // They will only work on lists of elements that respond to the
       // messages, but since C++ only instantiates the template if you use
       // you can still have Lists of ints, chars*, as long as you 
       // don't try to SetParentAll on that list.
    void SetParentAll(Node *p)
        { for (int i = 0; i < NumElements(); i++)
             Nth(i)->SetParent(p); }

};

#endif

