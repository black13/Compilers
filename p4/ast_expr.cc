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
//extern Type* inClass;
extern ClassDecl* inClass;
//extern SymbolTable *function;
extern FnDecl *function;
extern bool error;

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
    if (error) cout << "ArithmeticExpr::Emit()" << endl;
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // If the operator is '-' return 'right == false'
    return codeGen->GenBinaryOp("-", codeGen->GenLoadConstant(0), right->Emit(codeGen));
}

Type* RelationalExpr::GetType() {
    return Type::boolType;
}


Location* RelationalExpr::Emit(CodeGenerator *codeGen) {
    if (error) cout << "RelationalExpr::Emit()" << endl;
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

Location* EqualityExpr::Emit(CodeGenerator *codeGen) {
    if (error) cout << "EqualityExpr::Emit()" << endl;
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


Location* LogicalExpr::Emit(CodeGenerator *codeGen) {
    if (error) cout << "LogicalExpr::Emit()" << endl;
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // If the operator is '!' return 'right == false'
    return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), right->Emit(codeGen));
}

Type* AssignExpr::GetType() {
    if (error) cout << "AssignExpr::GetType()" << endl;
    return left->GetType();
}


Location* AssignExpr::Emit(CodeGenerator *codeGen) {
    if (error) cout << "AssignExpr::Emit()" << endl;
    Location *r = right->Emit(codeGen);

    // If we are assigning to a memory location, call EmitStore
    if (left->IsMemAccess()) left->EmitStore(codeGen, r);

    // Otherwise, just assign right to the left
    else codeGen->GenAssign(left->Emit(codeGen), r);

    return r;
}


Type* This::GetType() {
    return inClass->GetType();
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

Location* ArrayAccess::GetOffsetLocation(CodeGenerator* codeGen) {
    Location *varSize = codeGen->GenLoadConstant(CodeGenerator::VarSize);
    Location *offset = codeGen->GenBinaryOp("*", subscript->Emit(codeGen), varSize);
    Location *b = base->Emit(codeGen);

    // Check that index is >= 0 
    char *label0 = codeGen->NewLabel();
    char *label1 = codeGen->NewLabel();

    Location *test = codeGen->GenBinaryOp("<", offset, codeGen->GenLoadConstant(0));
    codeGen->GenIfZ(test, label0);
    codeGen->GenBuiltInCall(PrintString, codeGen->GenLoadConstant(err_arr_out_of_bounds));
    codeGen->GenBuiltInCall(Halt);
    
    // Check that index is below max bound
    codeGen->GenLabel(label0);
    Location *size = codeGen->GenBinaryOp("*", codeGen->GenLoad(b), varSize);
    test = codeGen->GenBinaryOp("||", codeGen->GenBinaryOp("<", size, offset), codeGen->GenBinaryOp("==", size, offset));
    codeGen->GenIfZ(test, label1);

    codeGen->GenBuiltInCall(PrintString, codeGen->GenLoadConstant(err_arr_out_of_bounds));
    codeGen->GenBuiltInCall(Halt);
    codeGen->GenLabel(label1);

    Location *location = codeGen->GenBinaryOp("+", base->Emit(codeGen), offset);

    //add varSize to the offset for the array header
    return codeGen->GenBinaryOp("+", location, varSize);
}

Location* ArrayAccess::Emit(CodeGenerator *codeGen) {
    if (error) cout << "ArrayAccess::Emit()" << endl;
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
    if (error) cout << "FieldAccess::GetType" << endl;
    if (!base) {
        Decl *decl = symbols->Search(field->GetName());
        if (decl) return decl->GetType();
        return NULL;
    }
    else {
        Decl *decl = symbols->Search(base->GetType()->GetName());
        Decl *func = decl->SearchScope(field->GetName());
        if (func) return func->GetType();
        return NULL;
    }
}


// TODO: This may not be right...
bool FieldAccess::IsMemAccess() {
    if (base || inClass) return true;
    return false;
}

Location* FieldAccess::EmitStore(CodeGenerator* codeGen, Location* val) {
    if (error) cout << "FieldAccess::EmitStore()" << endl;
    if (base) {
        if (error) cout << "FieldAccess::EmitStore(): Base" << endl;
        Decl *b = symbols->Search(base->GetName());

        Decl *klass = symbols->Search(base->GetType()->GetName());
        Decl *var = klass->SearchScope(field->GetName());
        
        klass = symbols->Search(base->GetName());

        Location *offset = codeGen->GenLoadConstant(var->GetOffset());
        Location *loc = codeGen->GenBinaryOp("+", klass->GetLoc(), offset);
        codeGen->GenStore(loc, val);

        //codeGen->GenStore(b->GetLoc(), val, var->GetOffset());
    }
    else {
        if (error) cout << "FieldAccess::EmitStore(): No Base" << endl;
        //Decl *decl = function->SearchHead(field->GetName());
        Decl *decl = function->SearchFormals(field->GetName());
        if (!decl && inClass) {
            decl = inClass->SearchMembers(field->GetName());
            if (!decl) decl = symbols->Search(field->GetName());
            //Decl *klass = symbols->Search((char*)"this");
            Decl *klass = function->SearchFormals((char*)"this");
            Location *offset = codeGen->GenLoadConstant(decl->GetOffset());
            Location *loc = codeGen->GenBinaryOp("+", klass->GetLoc(), offset);
            codeGen->GenStore(loc, val);
            //Location *loc = codeGen->GenLoad(klass->GetLoc(), decl->GetOffset());
            //codeGen->GenStore(klass->GetLoc(), val, decl->GetOffset());

            if (error) cout << "FieldAccess::Emit(): Exit 1" << endl;
        }
        decl = symbols->Search(field->GetName());
        Location *loc = decl->GetLoc();
        if (error) cout << "FieldAccess::Emit(): Exit 2" << endl;
    }
    return NULL;
}

Location* FieldAccess::Emit(CodeGenerator *codeGen) {
    if (error) cout << "FieldAccess::Emit()" << endl;

    if (!base) {
        if (error) cout << "FieldAccess::Emit(): No Base" << endl;

        //Decl *decl = function->SearchHead(field->GetName());
        Decl *decl = function->SearchFormals(field->GetName());
        //if (!decl && inClass) {
        if (!decl && inClass) {
            if (error) cout << "FieldAccess::Emit(): No Base in Class" << endl;
            decl = symbols->Search(field->GetName());
            //Decl *klass = symbols->Search((char*)"this");
            Decl *klass = function->SearchFormals((char*)"this");
            Location *loc = codeGen->GenLoad(klass->GetLoc(), decl->GetOffset());
            
            if (error) cout << "FieldAccess::Emit(): Exit 1" << endl;
            return loc;
        }

        if (error) cout << "FieldAccess::Emit(): No Base out of Class" << endl;
        decl = symbols->Search(field->GetName());
        Location *loc = decl->GetLoc();
        if (error) cout << "FieldAccess::Emit(): Exit 2" << endl;
        return loc;
    }
    else {
        if (error) cout << "FieldAccess::Emit(): With Base" << endl;

        Decl *b = symbols->Search(base->GetType()->GetName());
        Decl *decl = b->SearchScope(field->GetName());
        //Decl *decl = b->SearchMembers(field->GetName());
        if (decl) {
            Decl *param = symbols->Search(base->GetName());
            Location *loc = codeGen->GenLoad(param->GetLoc(), decl->GetOffset());

            if (error) cout << "FieldAccess::Emit(): Exit" << endl;
            return loc;
        }
    }

    if (error) cout << "FieldAccess::Emit(): Return NULL" << endl;
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

Location* Call::Emit(CodeGenerator *codeGen) {
    if (error) cout << "Call::Emit()" << endl;
    Location *result = NULL;
    int bytes = 0;
    // Tac instructions push params right to left.
    if (!base && !inClass) {
        if (error) cout << "Call::Emit(): !base && !inClass" << endl;
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
        if (error) cout << "Call::Emit(): !base && inClass" << endl;
        Decl *thiss = symbols->Search((char*)"this");
        Location *param = thiss->GetLoc();

        if (error) cout << "Call::Emit(): !base && inClass" << endl;
        Decl *func = symbols->Search(field->GetName());
        Location *load = codeGen->GenLoad(codeGen->GenLoad(param), func->GetOffset());

        if (error) cout << "Call::Emit(): !base && inClass" << endl;
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
        if (error) cout << "Call::Emit(): ArrayType" << endl;
        for (int i = actuals->NumElements() - 1; i >= 0; i--) {
            bytes += CodeGenerator::VarSize;
            Location *param = actuals->Nth(i)->Emit(codeGen);
            codeGen->GenPushParam(param);
        }
        result = codeGen->GenLoad(base->Emit(codeGen));
    }
    else {
        if (error) cout << "Call::Emit(): has base" << endl;

        Decl *klass = NULL;
        if (base->GetName()) klass = symbols->Search(base->GetName());

        Location *param = NULL;
        if (klass) param = klass->GetLoc();

        klass = symbols->Search(base->GetType()->GetName());
        Decl *func = klass->SearchScope(field->GetName());

        Location *b = base->Emit(codeGen);
        Location *loc = codeGen->GenLoad(b);
        Location *load = codeGen->GenLoad(loc, func->GetOffset());

        for (int i = actuals->NumElements() - 1; i >= 0; i--) {
            bytes += CodeGenerator::VarSize;
            Location *p = actuals->Nth(i)->Emit(codeGen);
            codeGen->GenPushParam(p);
        }
        if (param) codeGen->GenPushParam(param);
        else codeGen->GenPushParam(b);
        bytes += CodeGenerator::VarSize;

        if (error) cout << "Call::Emit(): has base: Gen ACall" << endl;
        if (func->GetType()->IsEquivalentTo(Type::voidType))
            result = codeGen->GenACall(load, false);
        else
            result = codeGen->GenACall(load, true);

    }

    codeGen->GenPopParams(bytes);
    if (error) cout << "Call::Emit(): Exit" << endl;
    return result;
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

Type* NewExpr::GetType() {
    return cType;
}

Location* NewExpr::Emit(CodeGenerator *codeGen) {
    if (error) cout << "NewExpr::Emit()" << endl;
    Decl *klass = symbols->Search(cType->GetName());
    int bytes = codeGen->GetOffset(); //TODO I'm not sure if this is correct (Ian)

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

Location* NewArrayExpr::Emit(CodeGenerator *codeGen) {
    if (error) cout << "NewArrayExpr::Emit()" << endl;
    Location *s = size->Emit(codeGen);
    
    // Check that size is > 0
    char *label = codeGen->NewLabel();

    Location *test = codeGen->GenBinaryOp("<", s, codeGen->GenLoadConstant(1));
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

Location* ReadIntegerExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadInteger);
}

Type* ReadLineExpr::GetType() {
    return Type::stringType;
}

Location* ReadLineExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadLine);
}
