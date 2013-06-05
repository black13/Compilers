/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "errors.h"

extern SymbolTable *symbols;
extern int labelNum;
extern Type* inClass;
extern SymbolTable *function;

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

int ArithmeticExpr::GetBytes() {
    if (left && right) return CodeGenerator::VarSize + left->GetBytes() + right->GetBytes();
    return (2 * CodeGenerator::VarSize) + right->GetBytes();
}

Location* ArithmeticExpr::Emit(CodeGenerator *codeGen) {
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // If the operator is '-' return 'right == false'
    return codeGen->GenBinaryOp("-", codeGen->GenLoadConstant(0), right->Emit(codeGen));
}

Type* RelationalExpr::GetType() {
    return Type::boolType;
}

int RelationalExpr::GetBytes() {
    int size = CodeGenerator::VarSize + left->GetBytes() + right->GetBytes(); 
    if (op->EqualTo("<="))
        return (2 * CodeGenerator::VarSize) + size;
    else if (op->EqualTo(">"))
        return (4 * CodeGenerator::VarSize) + size;
    else if (op->EqualTo(">="))
        return (2 * CodeGenerator::VarSize) + size;
    else
        return size;
}

Location* RelationalExpr::Emit(CodeGenerator *codeGen) {
    Location *l     = left->Emit(codeGen);
    Location *r     = right->Emit(codeGen);
    if (op->EqualTo("<"))
        return codeGen->GenBinaryOp("<", l, r);
    else if (op->EqualTo("<=")) {
        Location *equal = codeGen->GenBinaryOp("==", l, r);
        Location *less  = codeGen->GenBinaryOp("<", l, r);
        return codeGen->GenBinaryOp("||", less, equal);
    }
    else if (op->EqualTo(">")) {
        Location *equal = codeGen->GenBinaryOp("==", l, r);
        Location *less  = codeGen->GenBinaryOp("<", l, r);
        Location *leq   = codeGen->GenBinaryOp("||", less, equal);
        return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), leq);
    }
    else if (op->EqualTo(">=")) {
        Location *less  = codeGen->GenBinaryOp("<", l, r);
        return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), less);
    }

    return NULL;
}

Type* EqualityExpr::GetType() {
    return Type::boolType;
}

int EqualityExpr::GetBytes() {
    int size = CodeGenerator::VarSize + left->GetBytes() + right->GetBytes(); 
    if (op->EqualTo("=="))
        return size;
    return size + (2 * CodeGenerator::VarSize);
}

Location* EqualityExpr::Emit(CodeGenerator *codeGen) {
    Location *loc = NULL;
    if (left->GetType()->IsEquivalentTo(Type::stringType) && right->GetType()->IsEquivalentTo(Type::stringType)) {
        loc = codeGen->GenBuiltInCall(StringEqual, left->Emit(codeGen), right->Emit(codeGen));
    }
    else {
        loc = codeGen->GenBinaryOp("==", left->Emit(codeGen), right->Emit(codeGen));
    }

    if (op->EqualTo("=="))
        return loc;
    // If operator is '!=' reverse the result of '=='
    return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), loc);
}

Type* LogicalExpr::GetType() {
    return Type::boolType;
}

int LogicalExpr::GetBytes() {
    if (left && right) return CodeGenerator::VarSize + left->GetBytes() + right->GetBytes();
    return (2 * CodeGenerator::VarSize) + right->GetBytes();
}

Location* LogicalExpr::Emit(CodeGenerator *codeGen) {
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // If the operator is '!' return 'right == false'
    return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), right->Emit(codeGen));
}

Type* AssignExpr::GetType() {
    return left->GetType();
}

int AssignExpr::GetBytes() {
    return left->GetBytes() + right->GetBytes();
}

Location* AssignExpr::Emit(CodeGenerator *codeGen) {
    Location *r = right->Emit(codeGen);

    // If we are assigning to a memory location, call EmitStore
    if (left->IsMemAccess()) left->EmitStore(codeGen, r);

    // Otherwise, just assign right to the left
    else codeGen->GenAssign(left->Emit(codeGen), r);

    return r;
}


Type* This::GetType() {
    return inClass;
}

Location* This::Emit(CodeGenerator *codeGen) {
    Decl *decl = symbols->Search((char*)"this");
    if (decl) {
        return decl->GetLoc();
        //return codeGen->GenLoad(decl->GetLoc());
    }
    cout << "This::Emit: Returning NULL" << endl;
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
   
int CompoundExpr::GetBytes() {
    if (left && right) return CodeGenerator::VarSize + left->GetBytes() + right->GetBytes();
    return CodeGenerator::VarSize + right->GetBytes();
}

Location* CompoundExpr::Emit(CodeGenerator *codeGen) {
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // TODO: Handle case of one operand.
    return NULL;
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Location* ArrayAccess::EmitStore(CodeGenerator* codeGen, Location* val) {
    Location *saveloc = GetOffsetLocation(codeGen);
    Assert(val != NULL);
    codeGen->GenStore(saveloc,val);
    return codeGen->GenLoad(saveloc);
}

Type* ArrayAccess::GetType() {
    return base->GetType()->GetType();
}

int ArrayAccess::GetBytes() {
    return subscript->GetBytes() + base->GetBytes() + (6 * CodeGenerator::VarSize);
}

Location* ArrayAccess::GetOffsetLocation(CodeGenerator* codeGen) {
    Location *varSize = codeGen->GenLoadConstant(CodeGenerator::VarSize);
    Location *offset = codeGen->GenBinaryOp("*", subscript->Emit(codeGen), varSize);

    // Check that index is in bounds
    Location *test = codeGen->GenBinaryOp("<", offset, codeGen->GenLoadConstant(0));
    char label[80];
    sprintf(label, "_L%d", labelNum);
    labelNum++;
    codeGen->GenIfZ(test, label);
    codeGen->GenBuiltInCall(PrintString, codeGen->GenLoadConstant(err_arr_out_of_bounds));
    codeGen->GenBuiltInCall(Halt);
    codeGen->GenLabel(label);

    Location *location = codeGen->GenBinaryOp("+", base->Emit(codeGen), offset);

    //add varSize to the offset for the array header
    return codeGen->GenBinaryOp("+", location, varSize);
}

Location* ArrayAccess::Emit(CodeGenerator *codeGen) {
    return codeGen->GenLoad(GetOffsetLocation(codeGen));
}


FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

Type* FieldAccess::GetType() {
    if (!base) {
        Decl *func = symbols->Search(field->GetName());
        if (func) return func->GetType();
        return NULL;
    }
    else {
        Decl *decl = symbols->Search(base->GetType()->GetName());
        Decl *func = decl->SearchScope(field->GetName());
        if (func) return func->GetType();
        return NULL;
    }
}

int FieldAccess::GetBytes() {
    int n = 0;
    if (!base) {
        Decl *decl = function->SearchHead(field->GetName());
        if (!decl && inClass) {
            n += CodeGenerator::VarSize;
        }
        return n;
    }
    else {
        Decl *b = symbols->Search(base->GetType()->GetName());
        Decl *decl = b->SearchScope(field->GetName());
        if (decl) {
            n += CodeGenerator::VarSize;
        }
        return n;
    }
}

Location* FieldAccess::EmitStore(CodeGenerator* codeGen, Location* val) {
    Decl *b = symbols->Search(base->GetName());

    Decl *klass = symbols->Search(base->GetType()->GetName());
    Decl *var = klass->SearchScope(field->GetName());
    codeGen->GenStore(b->GetLoc(), val, var->GetOffset());
    return NULL;
    //return codeGen->GenLoad(b->GetLoc());
}

Location* FieldAccess::Emit(CodeGenerator *codeGen) {
    if (!base) {
        //Decl *decl = symbols->Search(field->GetName());
        Decl *decl = function->SearchHead(field->GetName());
        if (!decl && inClass) {
            decl = symbols->Search(field->GetName());
            Decl *klass = symbols->Search((char*)"this");
            Location *thiss = codeGen->GenLoad(klass->GetLoc(), decl->GetOffset());
            return thiss;
        }
        decl = symbols->Search(field->GetName());
        Location *loc = decl->GetLoc();
        return loc;
    }
    else {
        Decl *b = symbols->Search(base->GetType()->GetName());
        Decl *decl = b->SearchScope(field->GetName());
        if (decl) {
            Decl *param = symbols->Search(base->GetName());
            //Location *loc = codeGen->GenLoad(base->Emit(codeGen), decl->GetOffset());
            Location *loc = codeGen->GenLoad(param->GetLoc(), decl->GetOffset());
            return loc;
        }
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
 

Type* Call::GetType() {
    if (!base) {
        return field->GetType();
    }
    else if (base->GetType()->IsArrayType()) {
        if (strcmp(field->GetName(), "length") == 0)
            return Type::intType;
    }
    else {
        Decl *klass = symbols->Search(base->GetType()->GetName());
        Decl *func = klass->SearchScope(field->GetName());
        return func->GetType();
    }
    return NULL;
}

int Call::GetBytes(){
    int n = 0;
    for (int i = 0; i < actuals->NumElements(); i++) {
        n += actuals->Nth(i)->GetBytes();
    }

    //if (base) cout << base->GetType() << endl;
    if (!base && !inClass) {
        Decl *decl = symbols->Search(field->GetName());
        if (!decl->GetType()->IsEquivalentTo(Type::voidType)) {
            n += CodeGenerator::VarSize;
        }
        return n;
    }   
    else if (!base && inClass) {
        Decl *func = symbols->Search(field->GetName());
        if (!func->GetType()->IsEquivalentTo(Type::voidType)) {
            n += CodeGenerator::VarSize;
        }

        n += (1 * CodeGenerator::VarSize);
    }
    /*
    else if (base->GetType()->IsArrayType()) {
        n += 0;//base->GetBytes() + CodeGenerator::VarSize;
        return n;
    }
    */
    else {
    /*
        Decl *klass = symbols->Search(base->GetType()->GetName());
        if (klass) cout << "hit1" << endl;
        Decl *func = klass->SearchScope(field->GetName());
        if (func) cout << "hit2" << endl;
        if (!func->GetType()->IsEquivalentTo(Type::voidType)) {
            n += CodeGenerator::VarSize;
        }
        */

        n += (3 * CodeGenerator::VarSize);
    }
    return n;
}

Location* Call::Emit(CodeGenerator *codeGen) {
    Location *result = NULL;
    int bytes = 0;
    // Tac instructions push params right to left.
    if (!base && !inClass) {
        for (int i = actuals->NumElements() - 1; i >= 0; i--) {
            bytes += CodeGenerator::VarSize;
            Location *param = actuals->Nth(i)->Emit(codeGen);
            codeGen->GenPushParam(param);
        }
        Decl *decl = symbols->Search(field->GetName());
        if (decl->GetType()->IsEquivalentTo(Type::voidType)) {
            result = codeGen->GenLCall(field->GetName(), false);
        }
        else {
            result = codeGen->GenLCall(field->GetName(), true);
        }
    }
    else if (!base && inClass) {
        Decl *thiss = symbols->Search((char*)"this");
        Location *param = thiss->GetLoc();

        Decl *func = symbols->Search(field->GetName());
        Location *load = codeGen->GenLoad(codeGen->GenLoad(param), func->GetOffset());

        for (int i = actuals->NumElements() - 1; i >= 0; i--) {
            bytes += CodeGenerator::VarSize;
            Location *p = actuals->Nth(i)->Emit(codeGen);
            codeGen->GenPushParam(p);
        }
        codeGen->GenPushParam(param);
        bytes += CodeGenerator::VarSize;

        if (func->GetType()->IsEquivalentTo(Type::voidType))
            result = codeGen->GenACall(load, false);
        else
            result = codeGen->GenACall(load, true);
    }
    else if (base->GetType()->IsArrayType()) {
        for (int i = actuals->NumElements() - 1; i >= 0; i--) {
            bytes += CodeGenerator::VarSize;
            Location *param = actuals->Nth(i)->Emit(codeGen);
            codeGen->GenPushParam(param);
        }
        result = codeGen->GenLoad(base->Emit(codeGen));
    }
    else {
        Decl *klass = symbols->Search(base->GetName());
        Location *param = klass->GetLoc();
        klass = symbols->Search(base->GetType()->GetName());
        Decl *func = klass->SearchScope(field->GetName());
        Location *loc = codeGen->GenLoad(base->Emit(codeGen));
        Location *load = codeGen->GenLoad(loc, func->GetOffset());

/*
        if (func) {
            Decl *param = symbols->Search(base->GetName());
            Location *loc = codeGen->GenLoad(param->GetLoc(), decl->GetOffset());
            return loc;
        }
        */
        /*
        else {
            Decl *thiss = symbols->Search("this");
            param = thiss->GetLoc();

            func = symbols->Search(field->GetName());
            Location *load = codeGen->GenLoad(param, func->GetOffset());

        }
        */

        for (int i = actuals->NumElements() - 1; i >= 0; i--) {
            bytes += CodeGenerator::VarSize;
            Location *p = actuals->Nth(i)->Emit(codeGen);
            codeGen->GenPushParam(p);
        }
        codeGen->GenPushParam(param);
        bytes += CodeGenerator::VarSize;

        if (func->GetType()->IsEquivalentTo(Type::voidType))
            result = codeGen->GenACall(load, false);
        else
            result = codeGen->GenACall(load, true);

    }

    codeGen->GenPopParams(bytes);
    return result;
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

Type* NewExpr::GetType() {
    return cType;
}

int NewExpr::GetBytes() {
    return 3 * CodeGenerator::VarSize;
}

Location* NewExpr::Emit(CodeGenerator *codeGen) {
    Decl *klass = symbols->Search(cType->GetName());
    int bytes = klass->GetBytes();
    //cout << bytes << endl;

    Location *size = codeGen->GenLoadConstant(bytes);
    Location *ret = codeGen->GenBuiltInCall(Alloc, size);
    Location *label = codeGen->GenLoadLabel(cType->GetName());
    codeGen->GenStore(ret, label);
    return ret;
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Type* NewArrayExpr::GetType() {
    return elemType;
}

int NewArrayExpr::GetBytes() {//TODO this may not be correct
    return size->GetBytes() + (5 * CodeGenerator::VarSize);
}

Location* NewArrayExpr::Emit(CodeGenerator *codeGen) {
    Location *s = size->Emit(codeGen);
    
    // Check that size is > 0
    Location *test = codeGen->GenBinaryOp("<", s, codeGen->GenLoadConstant(1));
    char label[80];
    sprintf(label, "_L%d", labelNum);
    labelNum++;
    codeGen->GenIfZ(test, label);
    codeGen->GenBuiltInCall(PrintString, codeGen->GenLoadConstant(err_arr_bad_size));
    codeGen->GenBuiltInCall(Halt);
    codeGen->GenLabel(label);

    //multiply size times varsize for alloc
    Location * c = codeGen->GenLoadConstant(CodeGenerator::VarSize);
    Location * n = codeGen->GenBinaryOp("*",s,c);

    //add 1 to the size to store the array size
    Location * t = codeGen->GenBinaryOp("+",n,c);

    //allocate space
    Location * ret = codeGen->GenBuiltInCall(Alloc, t);

    //store the size as the first element
    //Assert(s != NULL);
    codeGen->GenStore(ret,s);

    return ret;
}


Type* ReadIntegerExpr::GetType() {
    return Type::intType;
}
int ReadIntegerExpr::GetBytes() {
    return CodeGenerator::VarSize;
}
Location* ReadIntegerExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadInteger);
}

Type* ReadLineExpr::GetType() {
    return Type::stringType;
}
int ReadLineExpr::GetBytes() {
    return CodeGenerator::VarSize;
}
Location* ReadLineExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadLine);
}
