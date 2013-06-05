/* File: ast_expr.h
 * ----------------
 * The Expr class and its subclasses are used to represent
 * expressions in the parse tree.  For each expression in the
 * language (add, call, New, etc.) there is a corresponding
 * node class for that construct. 
 *
 * pp4: You will need to extend the Expr classes to implement 
 * code generation for expressions.
 */


#ifndef _H_ast_expr
#define _H_ast_expr

#include "ast.h"
#include "ast_stmt.h"
#include "ast_type.h"
#include "list.h"

class NamedType; // for new
class Type; // for NewArray


class Expr : public Stmt 
{
  public:
    Expr(yyltype loc) : Stmt(loc) {}
    Expr() : Stmt() {}
    virtual int GetBytes() { return 0; }
    virtual Type* GetType() { return NULL; }
    virtual char* GetName() { return NULL; }
    virtual bool IsMemAccess() { return false; }
    virtual Location * EmitStore(CodeGenerator *codeGen, Location *right) { return NULL; } 
};

/* This node type is used for those places where an expression is optional.
 * We could use a NULL pointer, but then it adds a lot of checking for
 * NULL. By using a valid, but no-op, node, we save that trouble */
class EmptyExpr : public Expr
{
  public:
};

class IntConstant : public Expr 
{
  protected:
    int value;
  
  public:
    IntConstant(yyltype loc, int val);
    int GetBytes() { return CodeGenerator::VarSize; };
    Type* GetType() { return Type::intType; };
    Location* Emit(CodeGenerator *codeGen);
};

class DoubleConstant : public Expr 
{
  protected:
    double value;
    
  public:
    DoubleConstant(yyltype loc, double val);
    int GetBytes() { return CodeGenerator::VarSize; };
    Type* GetType() { return Type::doubleType; };
    //Location* Emit(CodeGenerator *codeGen);
};

class BoolConstant : public Expr 
{
  protected:
    bool value;
    
  public:
    BoolConstant(yyltype loc, bool val);
    int GetBytes() { return CodeGenerator::VarSize; };
    Type* GetType() { return Type::boolType; };
    Location* Emit(CodeGenerator *codeGen);
};

class StringConstant : public Expr 
{ 
  protected:
    char *value;
    
  public:
    StringConstant(yyltype loc, const char *val);
    int GetBytes() { return CodeGenerator::VarSize; };
    Type* GetType() { return Type::stringType; };
    Location* Emit(CodeGenerator *codeGen);
};

class NullConstant: public Expr 
{
  public: 
    NullConstant(yyltype loc) : Expr(loc) {}
    int GetBytes() { return CodeGenerator::VarSize; };
    Type* GetType() { return Type::nullType; };
    Location* Emit(CodeGenerator *codeGen);
};

class Operator : public Node 
{
  protected:
    char tokenString[4];
    
  public:
    Operator(yyltype loc, const char *tok);
    const char* GetName() { return tokenString; };
    bool EqualTo(const char* op) { return !strcmp(tokenString, op); };
    friend ostream& operator<<(ostream& out, Operator *o) { return out << o->tokenString; }
 };
 
class CompoundExpr : public Expr
{
  protected:
    Operator *op;
    Expr *left, *right; // left will be NULL if unary
    
  public:
    CompoundExpr(Expr *lhs, Operator *op, Expr *rhs); // for binary
    CompoundExpr(Operator *op, Expr *rhs);             // for unary
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class ArithmeticExpr : public CompoundExpr 
{
  public:
    ArithmeticExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    ArithmeticExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class RelationalExpr : public CompoundExpr 
{
  public:
    RelationalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class EqualityExpr : public CompoundExpr 
{
  public:
    EqualityExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    const char *GetPrintNameForNode() { return "EqualityExpr"; }
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class LogicalExpr : public CompoundExpr 
{
  public:
    LogicalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    LogicalExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    const char *GetPrintNameForNode() { return "LogicalExpr"; }
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class AssignExpr : public CompoundExpr 
{
  public:
    AssignExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    const char *GetPrintNameForNode() { return "AssignExpr"; }
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class LValue : public Expr 
{
  public:
    LValue(yyltype loc) : Expr(loc) {}
    Type* GetType() { return NULL; }
};

class This : public Expr 
{
  public:
    This(yyltype loc) : Expr(loc) {}
    Type* GetType();
    char * GetName() { return (char*)"this"; }
    Location* Emit(CodeGenerator *codeGen);
};

class ArrayAccess : public LValue 
{
  protected:
    Expr *base, *subscript;
    Location* GetOffsetLocation(CodeGenerator* codeGen);
    
  public:
    ArrayAccess(yyltype loc, Expr *base, Expr *subscript);
    Type* GetType();
    char* GetName() { return base->GetName(); }
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
    Location* EmitStore(CodeGenerator* codeGen, Location* loc);
    bool IsMemAccess() { if (base) return true; return false; }
};

/* Note that field access is used both for qualified names
 * base.field and just field without qualification. We don't
 * know for sure whether there is an implicit "this." in
 * front until later on, so we use one node type for either
 * and sort it out later. */
class FieldAccess : public LValue 
{
  protected:
    Expr *base;	// will be NULL if no explicit base
    Identifier *field;
    
  public:
    FieldAccess(Expr *base, Identifier *field); //ok to pass NULL base
    int GetOffset(CodeGenerator* codeGen);
    Type* GetType();
    int GetBytes();
    char* GetName() { return field->GetName(); }
    Location* Emit(CodeGenerator *codeGen);
    Location* EmitStore(CodeGenerator* codeGen, Location* loc);
    bool IsMemAccess(); 
};

/* Like field access, call is used both for qualified base.field()
 * and unqualified field().  We won't figure out until later
 * whether we need implicit "this." so we use one node type for either
 * and sort it out later. */
class Call : public Expr 
{
  protected:
    Expr *base;	// will be NULL if no explicit base
    Identifier *field;
    List<Expr*> *actuals;
    
  public:
    Call(yyltype loc, Expr *base, Identifier *field, List<Expr*> *args);
    int GetBytes();
    Type* GetType();
    Location* Emit(CodeGenerator *codeGen);
};

class NewExpr : public Expr
{
  protected:
    NamedType *cType;
    
  public:
    NewExpr(yyltype loc, NamedType *clsType);
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class NewArrayExpr : public Expr
{
  protected:
    Expr *size;
    Type *elemType;
    
  public:
    NewArrayExpr(yyltype loc, Expr *sizeExpr, Type *elemType);
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class ReadIntegerExpr : public Expr
{
  public:
    ReadIntegerExpr(yyltype loc) : Expr(loc) {}
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

class ReadLineExpr : public Expr
{
  public:
    ReadLineExpr(yyltype loc) : Expr (loc) {}
    Type* GetType();
    int GetBytes();
    Location* Emit(CodeGenerator *codeGen);
};

    
#endif
