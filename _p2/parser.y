/* File: parser.y
 * --------------
 * Bison input file to generate the parser for the compiler.
 *
 * pp2: your job is to write a parser that will construct the parse tree
 *      and if no parse errors were found, print it.  The parser should 
 *      accept the language as described in specification, and as augmented 
 *      in the pp2 handout.
 */

%{

/* Just like lex, the text within this first region delimited by %{ and %}
 * is assumed to be C/C++ code and will be copied verbatim to the y.tab.c
 * file ahead of the definitions of the yyparse() function. Add other header
 * file inclusions or C++ variable declarations/prototypes that are needed
 * by your code here.
 */
#include "scanner.h" // for yylex
#include "parser.h"
#include "errors.h"

void yyerror(const char *msg); // standard error-handling routine

%}

/* The section before the first %% is the Definitions section of the yacc
 * input file. Here is where you declare tokens and types, add precedence
 * and associativity options, and so on.
 */
 
/* yylval 
 * ------
 * Here we define the type of the yylval global variable that is used by
 * the scanner to store attibute information about the token just scanned
 * and thus communicate that information to the parser. 
 *
 * pp2: You will need to add new fields to this union as you add different 
 *      attributes to your non-terminal symbols.
 */


/* The type of these variables are from ast classes */
%union {
    int integerConstant;
    bool boolConstant;
    char *stringConstant;
    double doubleConstant;
    char identifier[MaxIdentLen+1]; // +1 for terminating null

    Decl *decl;
    VarDecl *varDecl;
    FnDecl *fnDecl;
    InterfaceDecl *intDecl;

    List<Decl*> *declList;
    List<VarDecl*> *varList;

    Type *type;
    Identifier *ident;
    NamedType *ntype;
    List<NamedType*> *impList;

    Stmt *stmt;
    List<Stmt*> *stmtList;
    
    Expr *expr;
    List<Expr*> *exprList;

    Case *caseLabel;
    List<Case*> *caseList;
    Default *defaultLabel;
}


/* Tokens
 * ------
 * Here we tell yacc about all the token types that we are using.
 * Bison will assign unique numbers to these and export the #define
 * in the generated y.tab.h header file.
 */
%token   T_Void T_Bool T_Int T_Double T_String T_Class 
%token   T_LessEqual T_GreaterEqual T_Equal T_NotEqual T_Dims
%token   T_And T_Or T_Null T_Extends T_This T_Interface T_Implements
%token   T_While T_For T_If T_Else T_Return T_Break
%token   T_New T_NewArray T_Print T_ReadInteger T_ReadLine
%token   T_Increment T_Decrement T_Switch T_Case T_Default

%token   <identifier> T_Identifier
%token   <stringConstant> T_StringConstant 
%token   <integerConstant> T_IntConstant
%token   <doubleConstant> T_DoubleConstant
%token   <boolConstant> T_BoolConstant


/* Non-terminal types
 * ------------------
 * In order for yacc to assign/access the correct field of $$, $1, we
 * must to declare which field is appropriate for the non-terminal.
 * As an example, this first type declaration establishes that the DeclList
 * non-terminal uses the field named "declList" in the yylval union. This
 * means that when we are setting $$ for a reduction for DeclList ore reading
 * $n which corresponds to a DeclList nonterminal we are accessing the field
 * of the union named "declList" which is of type List<Decl*>.
 * pp2: You'll need to add many of these of your own.
 */
%type <decl>      Decl ClassDecl Prototype Field
%type <declList>  DeclList ProtoList FieldList

%type <intDecl>   InterfaceDecl
%type <fnDecl>    FunctionDecl
%type <varDecl>   VarDecl Variable
%type <varList>   VarList Formals VarDeclList 

%type <type>      Type
%type <ident>     Identifier
%type <ntype>     NType
%type <impList>   ImpList OptionalImp

%type <stmt>      Stmt StmtBlock BreakStmt PrintStmt ReturnStmt IfStmt ForStmt WhileStmt SwitchStmt
%type <stmtList>  StmtList 

%type <expr>      Constant OptionalExpr Call Expr LValue
%type <exprList>  ExprList Actuals

%type <caseLabel> Case
%type <caseList>  CaseList
%type <defaultLabel> Default

%nonassoc '='
%nonassoc NOELSE
%nonassoc T_Else
%right T_And T_Or
%right T_Equal T_NotEquals T_GreaterEqual T_LessEqual '<' '>'
%left '+' '-'
%left '*' '/' '%'
%left NEG

%%
/* Rules
 * -----
 * All productions and actions should be placed between the start and stop
 * %% markers which delimit the Rules section.
         
 */
Program       :   DeclList              { 
                                          Program *program = new Program($1);
                                          // if no errors, advance to next phase
                                          if (ReportError::NumErrors() == 0) 
                                              program->Print(0);
                                        }
              ;

DeclList      :   DeclList Decl         { ($$=$1)->Append($2); }
              |   Decl                  { ($$=new List<Decl*>)->Append($1); }
              ;

Decl          :   VarDecl               { $$=$1; }
              |   InterfaceDecl         { $$=$1; }
              |   FunctionDecl          { $$=$1; }
              |   ClassDecl             { $$=$1; }
              ;

ClassDecl     :   T_Class Identifier OptionalImp '{' FieldList '}'  { $$=new ClassDecl($2,NULL,$3,$5); }
              |   T_Class Identifier T_Extends NType OptionalImp '{' FieldList '}'  { $$=new ClassDecl($2,$4,$5,$7); }   
              ;

OptionalImp   :   ImpList               { $$=$1; }
              |   /* empty */           { $$=new List<NamedType*>; }
              ;

FieldList     :   FieldList Field       { ($$=$1)->Append($2); }
              |   /* empty */           { ($$=new List<Decl*>); }
              ;

Field         :   VarDecl               { $$=$1; } 
              |   FunctionDecl          { $$=$1; }
              ;

InterfaceDecl :   T_Interface Identifier '{' ProtoList '}' { $$=new InterfaceDecl($2,$4); }
              ;

FunctionDecl  :   Type Identifier '(' Formals ')' StmtBlock { ($$=new FnDecl($2,$1,$4))->SetFunctionBody($6); }
              |   T_Void Identifier '(' Formals ')' StmtBlock { ($$=new FnDecl($2,new Type("void"),$4))->SetFunctionBody($6); }
              ;

StmtBlock     :   '{' VarDeclList StmtList '}' { $$=new StmtBlock($2,$3); }
              ;

// I'm unclear why, but parser breaks if we don't have the second part of this rule, unlike other rules.
StmtList      :   StmtList Stmt         { ($$=$1)->Append($2); }
              |   Stmt                  { ($$=new List<Stmt*>)->Append($1); }
              |   /* empty */           { $$=new List<Stmt*>; }
              ;

VarDeclList   :   VarDeclList VarDecl   { ($$=$1)->Append($2); }
              |   /* empty */           { ($$=new List<VarDecl*>); }
              ;

VarDecl       :   Variable ';'          { $$=$1; }
              ;

Variable      :   Type Identifier       { $$=new VarDecl($2,$1); }
              ;
              
Stmt          :   OptionalExpr ';'      { $$=$1; }
              |   IfStmt                { $$=$1; }
              |   WhileStmt             { $$=$1; }
              |   ForStmt               { $$=$1; }
              |   BreakStmt             { $$=$1; }
              |   PrintStmt             { $$=$1; }
              |   ReturnStmt            { $$=$1; }
              |   SwitchStmt            { $$=$1; }
              |   StmtBlock             { $$=$1; }
              ;

ProtoList     :   ProtoList Prototype   { ($$=$1)->Append($2); }
              |   /* empty */           { ($$=new List<Decl*>); }
              ;
Prototype     :   Type Identifier '(' Formals ')' ';'   { $$=new FnDecl($2,$1,$4); }
              |   T_Void Identifier '(' Formals ')' ';' { $$=new FnDecl($2,new Type("void"),$4); }
              ;

Formals       :   VarList               { $$=$1; }
              |   /* empty */           { ($$=new List<VarDecl*>); }
              ;

VarList       :   VarList ',' Variable  { ($$=$1)->Append($3); }
              |   Variable              { ($$=new List<VarDecl*>)->Append($1); }
              ;

IfStmt        :   T_If '(' Expr ')' Stmt %prec NOELSE { $$=new IfStmt($3,$5,NULL); }
              |   T_If '(' Expr ')' Stmt T_Else Stmt  { $$=new IfStmt($3,$5,$7); }
              ;

WhileStmt     :   T_While '(' Expr ')' Stmt     { $$=new WhileStmt($3,$5); }
              ;

ForStmt       :   T_For '(' OptionalExpr ';' Expr ';' OptionalExpr ')' Stmt { $$=new ForStmt($3,$5,$7,$9); }
              ;

PrintStmt     :   T_Print '(' ExprList ')' ';'  { $$=new PrintStmt($3); }
              ;

BreakStmt     :   T_Break ';'                   { $$=new BreakStmt(@1); }
              ;

ReturnStmt    :   T_Return OptionalExpr ';'     { $$=new ReturnStmt(@1,$2); }
              ;

SwitchStmt    :   T_Switch '(' Expr ')' '{' CaseList Default '}'  { $$=new SwitchStmt($3,$6,$7); }
              ;

CaseList      :   CaseList Case         { ($$=$1)->Append($2); }
              |   Case                  { ($$=new List<Case*>)->Append($1); }
              ;

Case          :   T_Case T_IntConstant ':' StmtList  { $$=new Case(new IntConstant(@2,$2),$4); }
              ;

Default       :   T_Default ':' StmtList  { $$=new Default($3); }
              ;

OptionalExpr  :   Expr                  { $$=$1; }
              |   /* empty */           { $$=new EmptyExpr(); }
              ;

ExprList      :   ExprList ',' Expr     { ($$=$1)->Append($3); }
              |   Expr                  { ($$=new List<Expr*>)->Append($1); }
              ;

Expr          :   LValue '=' Expr       { $$=new AssignExpr($1,new Operator(@2,"="),$3); }   
              |   '(' Expr ')'          { $$=$2; }
              |   Expr '+' Expr         { $$=new ArithmeticExpr($1,new Operator(@2,"+"),$3); }
              |   Expr '-' Expr         { $$=new ArithmeticExpr($1,new Operator(@2,"-"),$3); }
              |   Expr '*' Expr         { $$=new ArithmeticExpr($1,new Operator(@2,"*"),$3); }
              |   Expr '/' Expr         { $$=new ArithmeticExpr($1,new Operator(@2,"/"),$3); }
              |   Expr '%' Expr         { $$=new ArithmeticExpr($1,new Operator(@2,"%"),$3); }
              |   '-' Expr %prec NEG    { $$=new ArithmeticExpr(new Operator(@1,"-"),$2); }
              |   Expr T_And Expr       { $$=new LogicalExpr($1,new Operator(@2,"&&"),$3); }
              |   Expr T_Or Expr        { $$=new LogicalExpr($1,new Operator(@2,"||"),$3); }
              |   Expr '<' Expr         { $$=new RelationalExpr($1,new Operator(@2,"<"),$3); }
              |   Expr T_LessEqual Expr { $$=new RelationalExpr($1,new Operator(@2,"<="),$3); }
              |   Expr '>' Expr         { $$=new RelationalExpr($1,new Operator(@2,">"),$3); }
              |   Expr T_GreaterEqual Expr  { $$=new RelationalExpr($1,new Operator(@2,">="),$3); }
              |   Expr T_Equal Expr     { $$=new EqualityExpr($1,new Operator(@2,"=="),$3); }
              |   Expr T_NotEqual Expr  { $$=new EqualityExpr($1,new Operator(@2,"!="),$3); }
              |   '!' Expr              { $$=new LogicalExpr(new Operator(@1, "!"), $2); }
              |   LValue T_Increment    { $$=new PostfixExpr($1, new Operator(@2, "++")); }
              |   LValue T_Decrement    { $$=new PostfixExpr($1, new Operator(@2, "--")); }
              |   T_ReadInteger '(' ')' { $$=new ReadIntegerExpr(@1); }
              |   T_ReadLine '(' ')'    { $$=new ReadLineExpr(@1); }
              |   T_New Identifier      { $$=new NewExpr(@2,new NamedType($2)); }
              |   T_NewArray '(' Expr ',' Type ')'  { $$=new NewArrayExpr(@1,$3,$5); }
              |   LValue                { $$=$1; }
              |   T_This                { $$=new This(@1); }
              |   Call                  { $$=$1; }
              |   Constant              { $$=$1; }
              ; 

LValue        :   Identifier            { $$=new FieldAccess(NULL,$1); }
              |   Expr '.' Identifier   { $$=new FieldAccess($1,$3); }
              |   Expr '[' Expr ']'     { $$=new ArrayAccess(@1,$1,$3); }
              ;

Call          :   Identifier '(' Actuals ')'          { $$=new Call(@1,NULL,$1,$3); }
              |   Expr '.' Identifier '(' Actuals ')' { $$=new Call(@1,$1,$3,$5); }  //call location may be wrong
              ;

ImpList       :   T_Implements ImpList  { $$=$2; }
              |   ImpList ',' NType     { ($$=$1)->Append($3); }
              |   NType                 { ($$=new List<NamedType*>)->Append($1); }
              ;

Actuals       :   ExprList              { $$=$1; }
              |   /* empty */           { $$=new List<Expr*>; }
              ;

Type          :   T_Int                 { $$=new Type("int"); }
              |   T_Double              { $$=new Type("double"); }
              |   T_Bool                { $$=new Type("bool"); }
              |   T_String              { $$=new Type("string"); }
              |   Identifier            { $$=new NamedType($1); }
              |   Type T_Dims           { $$=new ArrayType(@1,$1); }
              ;

NType         :   Identifier            { $$=new NamedType($1); }
              ;

Identifier    :   T_Identifier          { $$=new Identifier(@1,$1); }
              ;

Constant      :   T_IntConstant         { $$=new IntConstant(@1,$1);    }
              |   T_DoubleConstant      { $$=new DoubleConstant(@1,$1); }
              |   T_BoolConstant        { $$=new BoolConstant(@1,$1);   }
              |   T_StringConstant      { $$=new StringConstant(@1,$1); }
              |   T_Null                { $$=new NullConstant(@1); }
              ;

%%

/* The closing %% above marks the end of the Rules section and the beginning
 * of the User Subroutines section. All text from here to the end of the
 * file is copied verbatim to the end of the generated y.tab.c file.
 * This section is where you put definitions of helper functions.
 */

/* Function: InitParser
 * --------------------
 * This function will be called before any calls to yyparse().  It is designed
 * to give you an opportunity to do anything that must be done to initialize
 * the parser (set global variables, configure starting state, etc.). One
 * thing it already does for you is assign the value of the global variable
 * yydebug that controls whether yacc prints debugging information about
 * parser actions (shift/reduce) and contents of state stack during parser.
 * If set to false, no information is printed. Setting it to true will give
 * you a running trail that might be helpful when debugging your parser.
 * Please be sure the variable is set to false when submitting your final
 * version.
 */
void InitParser()
{
   PrintDebug("parser", "Initializing parser");
   yydebug = false;
}
