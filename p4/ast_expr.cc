/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

Location* IntConstant::Emit(CodeGenerator* codeGen) {
  return codeGen->GenLoadConstant(value);
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

Location* BoolConstant::Emit(CodeGenerator *codeGen) {
    return codeGen->GenLoadConstant(value ? 1 : 0);
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Location* StringConstant::Emit(CodeGenerator *codeGen) {
    return codeGen->GenLoadConstant(value);
}

Location* NullConstant::Emit(CodeGenerator *codeGen) {
    return codeGen->GenLoadConstant(0);
}


Type* ArithmeticExpr::GetType() {
    return right->GetType();
}

Location* ArithmeticExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}

Type* RelationalExpr::GetType() {
    return Type::boolType;
}

Location* RelationalExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}

Type* EqualityExpr::GetType() {
    return Type::boolType;
}

Location* EqualityExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}

Type* LogicalExpr::GetType() {
    return Type::boolType;
}

Location* LogicalExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}

Type* AssignExpr::GetType() {
    return left->GetType();
}

Location* AssignExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}


Type* This::GetType() {
    //TODO
    cout << "this::GetType:TODO" << endl;
    return NULL;
}

Location* This::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}
   
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Location* ArrayAccess::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}

Type* ArrayAccess::GetType() {
    return base->GetType();
}

Type* FieldAccess::GetType() {
    //TODO
    cout << "FieldAccess::GetType:TODO" << endl;
    return NULL;
}


Location* FieldAccess::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}

FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}
 

Type* Call::GetType() {
    //TODO
    cout << "Call::GetType:TODO" << endl;
    return NULL;
}

int Call::GetBytes(){
  cout << "Call::GetBytes:TODO" << endl;
  return 0;
}

Location* Call::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Call::Emit:TODO" << endl;
  return NULL;
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

Type* NewExpr::GetType() {
    //TODO
    cout << "NewExpr::Type:TODO" << endl;
    return NULL;
}


Location* NewExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "NewExpr::Emit:TODO" << endl;
  return NULL;
}


//TODO not sure if this is right
int NewExpr::GetBytes() {
    return 5 * CodeGenerator::VarSize;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Type* NewArrayExpr::GetType() {
    //TODO
    cout << "NewArr::Type:TODO" << endl;
    return NULL;
}

Location* NewArrayExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "Expr::Emit:TODO" << endl;
  return NULL;
}


Type* ReadIntegerExpr::GetType() {
    return Type::intType;
}

Location* ReadIntegerExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadInteger);
}

Type* ReadLineExpr::GetType() {
    return Type::stringType;
}

Location* ReadLineExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadLine);
}
