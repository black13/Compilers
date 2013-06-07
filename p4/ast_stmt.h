/* File: ast_stmt.h
 * ----------------
 * The Stmt class and its subclasses are used to represent
 * statements in the parse tree.  For each statment in the
 * language (for, if, return, etc.) there is a corresponding
 * node class for that construct. 
 *
 * pp4: You will need to extend the Stmt classes to implement
 * code generation for statements.
 */


#ifndef _H_ast_stmt
#define _H_ast_stmt

#include "list.h"
#include "ast.h"
#include "codegen.h"

class Decl;
class VarDecl;
class Expr;
class SymbolTable;
  
class Program : public Node
{
  protected:
     List<Decl*> *decls;
     CodeGenerator * codeGen;
     
  public:
     Program(List<Decl*> *declList);
     void Check();
     void Emit();
};

class Stmt : public Node
{
  protected:
    SymbolTable *scope; 

  public:
    Stmt() : Node() {}
    Stmt(yyltype loc) : Node(loc) {}
    // returns the size in bytes of the object
    virtual void AddSymbols() {}
};

class StmtBlock : public Stmt 
{
  protected:
    List<VarDecl*> *decls;
    List<Stmt*> *stmts;
    
  public:
    StmtBlock(List<VarDecl*> *variableDeclarations, List<Stmt*> *statements);
    Location* Emit(CodeGenerator* codeGen);
    void AddSymbols(); 

};

  
class ConditionalStmt : public Stmt
{
  protected:
    Expr *test;
    Stmt *body;
  
  public:
    ConditionalStmt(Expr *testExpr, Stmt *body);
};

class LoopStmt : public ConditionalStmt 
{
  protected:
    char *breakLabel;

  public:
    LoopStmt(Expr *testExpr, Stmt *body)
            : ConditionalStmt(testExpr, body) {}
    bool IsLoop() { return true; }
    char * GetBreakLabel() { return breakLabel; }
};

class ForStmt : public LoopStmt 
{
  protected:
    Expr *init, *step;
  
  public:
    ForStmt(Expr *init, Expr *test, Expr *step, Stmt *body);
    void AddSymbols();
    Location* Emit(CodeGenerator* codeGen);
};

class WhileStmt : public LoopStmt 
{
  public:
    WhileStmt(Expr *test, Stmt *body) : LoopStmt(test, body) {}
    Location* Emit(CodeGenerator* codeGen);
    void AddSymbols();
};

class IfStmt : public ConditionalStmt 
{
  protected:
    Stmt *elseBody;
  
  public:
    IfStmt(Expr *test, Stmt *thenBody, Stmt *elseBody);
    Location* Emit(CodeGenerator* codeGen);
    void AddSymbols();
};

class BreakStmt : public Stmt 
{
  public:
    BreakStmt(yyltype loc) : Stmt(loc) {}
    Location* Emit(CodeGenerator* codeGen);
};

class ReturnStmt : public Stmt  
{
  protected:
    Expr *expr;
  
  public:
    ReturnStmt(yyltype loc, Expr *expr);
    Location* Emit(CodeGenerator* codeGen);
};

class PrintStmt : public Stmt
{
  protected:
    List<Expr*> *args;
    
  public:
    PrintStmt(List<Expr*> *arguments);
    Location* Emit(CodeGenerator* codeGen);
};


#endif
