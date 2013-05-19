/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"

extern Type *classType;

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
   

Type* ArithmeticExpr::CheckType() {
  Type *lhs = NULL;
  Type *rhs = NULL;
  if (left) lhs = left->CheckType();
  if (right) rhs = right->CheckType();
  if (lhs && rhs) {
      if (lhs->ConvertableTo(rhs))
          return lhs;
      ReportError::IncompatibleOperands(op, lhs, rhs);
  }
  return NULL;
}

Type* RelationalExpr::CheckType() {
    Type *lhs = NULL; 
    Type *rhs = NULL;
    if (left) lhs = left->CheckType();
    if (right) rhs = right->CheckType();
    if (rhs && lhs) {
        if (rhs->EqualType(lhs))
            return Type::boolType;
        ReportError::IncompatibleOperands(op, lhs, rhs);
    }
    
    return NULL;
}

Type* EqualityExpr::CheckType() {
    Type *lhs = NULL; 
    Type *rhs = NULL;
    if (left) lhs = left->CheckType();
    if (right) rhs = right->CheckType();
    if (rhs && lhs) {
        if (!rhs->ConvertableTo(lhs) && !lhs->ConvertableTo(rhs))
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
        if (!rhs->EqualType(Type::boolType) && lhs->EqualType(Type::boolType))
            ReportError::IncompatibleOperands(op, lhs, rhs);
    }
    else if (rhs) {
        if (!rhs->EqualType(Type::boolType))
            ReportError::IncompatibleOperand(op, rhs);
    }
    
    return Type::boolType;
}
  
Type* AssignExpr::CheckType() {
    Type *lhs = NULL; 
    Type *rhs = NULL;
    if (left) lhs = left->CheckType();
    if (right) rhs = right->CheckType();
    if (rhs && lhs) {
        if (lhs->ConvertableTo(rhs))
            return lhs;
        ReportError::IncompatibleOperands(op, lhs, rhs);
    }
    
    return NULL;
}
  
Type* This::CheckType() {
    if (!classType) ReportError::ThisOutsideClassScope(this);
    return classType;
}


ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Type* ArrayAccess::CheckType() {
    // Check if access index is int
    Type * type = subscript->CheckType();
    if (type) {
        if (!type->EqualType(Type::intType))
            ReportError::SubscriptNotInteger(subscript);
    }

    // Get type of base and return
    if (base) {
        type = base->CheckType();
        if (dynamic_cast<ArrayType*>(type))
            return type;
        ReportError::BracketsOnNonArray(base);
    }
    return NULL;
}

FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

Type* FieldAccess::CheckType() {
    // No base, this is a variable access
    if (!base) {
        return field->CheckType(LookingForVariable);
    }
    if (dynamic_cast<This*>(base)) {
        return field->CheckType(LookingForVariable);
    }
    else if (!dynamic_cast<NamedType*>(base->CheckType())) {
        ReportError::FieldNotFoundInBase(field, base->CheckType());
    }
    return NULL;
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}
 
Type* Call::CheckType() {
    // Because we need to output the error that the variable doesn't exist before we check if the function exists.
    for (int i = 0; i < actuals->NumElements(); i++) {
        Type *given = actuals->Nth(i)->CheckType();
    }
    if (!base) {
        Type *returnType = field->CheckType(LookingForFunction);
        FnDecl *function = field->GetFunction();
        if (function) {
            List<VarDecl*> *formals = function->GetFormals();
            if (actuals->NumElements() != formals->NumElements())
                ReportError::NumArgsMismatch(field, formals->NumElements(), actuals->NumElements());
            else {
                for (int i = 0; i < actuals->NumElements(); i++) {
                    Type *given = actuals->Nth(i)->CheckType();
                    Type *expected = formals->Nth(i)->GetType();
                    if (!given->ConvertableTo(expected))
                        ReportError::ArgMismatch(actuals->Nth(i), i+1, given, expected);
                }
            }
            return returnType;
        }
        return NULL;
    }
    else if (dynamic_cast<This*>(base)) {
        return field->CheckType(LookingForFunction);
    }
    else if (dynamic_cast<ArrayType*>(base->CheckType())) {
        if (strcmp(field->GetName(),"length") != 0)
            ReportError::FieldNotFoundInBase(field, base->CheckType());
    }
    // SegFault
    //
    // If the Type is not a NamedType, then it is a primative that has no fields; int, double, etc.
    else if (dynamic_cast<NamedType*>(base->CheckType()) == NULL) {
        Type *type = base->CheckType();
        if (type) ReportError::FieldNotFoundInBase(field, type);
    }
    else {
        Type *type = base->CheckType();
        if (type) {
            ClassDecl *klass = type->GetClass();
            if (klass) {
                //Decl *decl = klass->CheckMember(field);
                //if (!decl) ReportError::FieldNotFoundInBase(field, type);
            }
        }
    }

    return NULL;
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
    Assert(c != NULL);
    (cType=c)->SetParent(this);
}

Type* NewExpr::CheckType() {
    Type *type = cType->CheckType(LookingForClass);
    return type;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Type* NewArrayExpr::CheckType() {
    // Check if array size is int
    Type *type = size->CheckType();
    if (type) {
        if (!type->EqualType(Type::intType))
            ReportError::NewArraySizeNotInteger(size);
    }
    type = NULL;
    if (elemType) type = elemType->CheckType(LookingForType);
    if (type) return new ArrayType(elemType);

    // This is a new declaration, so just return nullType
    return NULL;
}
