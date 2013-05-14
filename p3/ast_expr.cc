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

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
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
   

/*
Type* CompoundExpr::CheckType() {
  return Type::nullType;
  Type *leftType = NULL;
  if (left) leftType = left->CheckType();
  Type *rightType = NULL;
  if (right) rightType = right->CheckType();
  if (leftType && rightType) {
    if (leftType->EqualType(rightType)) {
      return leftType;
    }
    ReportError::IncompatibleOperands(op, leftType, rightType);
  }
  return Type::nullType;
}
*/

Type* RelationalExpr::CheckType() {
    Type *lhs = NULL; 
    Type *rhs = NULL;
    if (left) lhs = left->CheckType();
    if (right) rhs = right->CheckType();
    if (rhs && lhs) {
        if (!rhs->EqualType(lhs))
            ReportError::IncompatibleOperands(op, lhs, rhs);
    }
    
    return Type::boolType;
}

Type* EqualityExpr::CheckType() {
    Type *lhs = NULL; 
    Type *rhs = NULL;
    if (left) lhs = left->CheckType();
    if (right) rhs = right->CheckType();
    if (rhs && lhs) {
        if (!rhs->EqualType(lhs))
            ReportError::IncompatibleOperands(op, lhs, rhs);
    }
    
    return Type::boolType;
}

Type* LogicalExpr::CheckType() {
    Type *lhs = NULL; 
    Type *rhs = NULL;
    if (left) lhs = left->CheckType();
    if (right) rhs = right->CheckType();
    if (rhs && lhs) {
        if (!rhs->EqualType(Type::boolType) || !lhs->EqualType(Type::boolType))
            ReportError::IncompatibleOperands(op, lhs, rhs);
    }
    else {
        if (!rhs->EqualType(Type::boolType))
            ReportError::IncompatibleOperand(op, rhs);
    }
    
    return Type::boolType;
}
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Type* ArrayAccess::CheckType() {
    // Check if access index is int
    Type * temp = subscript->CheckType();
    if (temp) {
        if (!temp->EqualType(Type::intType))
            ReportError::SubscriptNotInteger(subscript);
    }

    // Get type of base and return
    return base->CheckType();
}

FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

Type* FieldAccess::CheckType() {
    return field->GetType();
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}
 
// TODO: Implement case for Expr.Identifier()
/*
Type* Call::CheckType() {
    return field->GetType();
}
*/

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Type* NewArrayExpr::CheckType() {
    // Check if array size is int
    Type *temp = size->CheckType();
    if (temp) {
        if (!temp->EqualType(Type::intType))
            ReportError::NewArraySizeNotInteger(size);
    }

    // This is a new declaration, so just return nullType
    return Type::nullType;
}
