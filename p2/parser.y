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
%union {
    int integerConstant;
    bool boolConstant;
    char *stringConstant;
    double doubleConstant;
    char identifier[MaxIdentLen+1]; // +1 for terminating null
    Decl *decl;
    List<Decl*> *declList;
    Type *type;
    Identifier *ident;
    InterfaceDecl *intDecl;
    VarDecl *varDecl;
    List<VarDecl*> *varList;
    FnDecl *fnDecl;
    Stmt *stmt;
    List<Stmt*> *stmtList;
    Expr *expr;
    List<Expr*> *exprList;
    LValue *lvalue;
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
%type <declList>  DeclList ProtoList
%type <decl>      Decl Prototype
%type <intDecl>   InterfaceDecl
%type <varDecl>   VarDecl Variable
%type <fnDecl>    FunctionDecl
%type <varList>   VarList Formals VarDeclList
%type <stmt>      Stmt StmtBlock BreakStmt PrintStmt ReturnStmt
%type <stmtList>  StmtList
%type <type>      Type
%type <ident>     Identifier
%type <lvalue>    LValue
%type <exprList>  ExprList Actuals
%type <expr>      Constant Expr OptionalExpr

%%
/* Rules
 * -----
 * All productions and actions should be placed between the start and stop
 * %% markers which delimit the Rules section.
	 
 */
Program       :   DeclList              { 
                                          /* pp2: The @1 is needed to convince 
                                           * yacc to set up yylloc. You can remove 
                                           * it once you have other uses of @n*/
                                          Program *program = new Program($1);
                                          // if no errors, advance to next phase
                                          if (ReportError::NumErrors() == 0) 
                                              program->Print(0);
                                        }
              ;

DeclList      :   DeclList Decl         { ($$=$1)->Append($2); }
              |   Decl                  { ($$=new List<Decl*>)->Append($1); }
              ;

Decl          :   VarDecl               { $$ = $1; }
              |   InterfaceDecl         { $$ = $1; }
              |   FunctionDecl          { $$ = $1; }
              ;

VarDecl       :   Variable ';'          { $$ = $1; }
              ;

InterfaceDecl :   T_Interface Identifier '{' ProtoList '}' { $$ = new InterfaceDecl($2,$4); }
              ;

FunctionDecl  :   Type Identifier '(' Formals ')' StmtBlock {
                                          $$ = new FnDecl($2,$1,$4);
                                          ($$)->SetFunctionBody($6);
                                        }
              |   T_Void Identifier '(' Formals ')' StmtBlock {
                                          Type *id = new Type("void");
                                          $$ = new FnDecl($2,id,$4);
                                          ($$)->SetFunctionBody($6);
                                        }
              ;

StmtBlock     :   '{' VarDeclList StmtList '}' {
                                          $$ = new StmtBlock($2,$3);
                                        }
              ;

VarDeclList   :   VarDeclList VarDecl   { ($$=$1)->Append($2); }
              |                         { ($$=new List<VarDecl*>); }
              ;

StmtList      :   StmtList Stmt         { ($$=$1)->Append($2); }
              |                         { ($$=new List<Stmt*>); }
              ;

Stmt          :   BreakStmt             { $$=$1; }
              |   PrintStmt             { $$=$1; }
              |   ReturnStmt            { $$=$1; }
              ;

BreakStmt     :   T_Break ';'           { $$=new BreakStmt(@1); }
              ;

ProtoList     :   ProtoList Prototype   { ($$=$1)->Append($2); }
              |                         { ($$=new List<Decl*>); }
              ;

Prototype     :   Type Identifier '(' Formals ')' ';'   {  }
              |   T_Void Identifier '(' Formals ')' ';' {  }
              ;

Formals       :   VarList               { $$=$1; }
              |   /* empty */           { ($$=new List<VarDecl*>); }
              ;

VarList       :   VarList ',' Variable  { ($$=$1)->Append($3); }
              |   Variable              { ($$=new List<VarDecl*>)->Append($1); }
              ;


Variable      :   Type Identifier       { $$ = new VarDecl($2,$1); }
              ;
              
Type          :   T_Int                 { $$ = new Type("int"); }
              |   T_Double              { $$ = new Type("double"); }
              |   T_Bool                { $$ = new Type("bool"); }
              |   T_String              { $$ = new Type("string"); }
              |   Identifier            { $$ = new NamedType($1); }
/* TODO: Array not working */
// Works for input 'int[ ] a;' but not 'int[] a;'
              |   Type '[' ']'          { $$ = new ArrayType(@1,$1); }
              ;

Identifier    :   T_Identifier          { $$ = new Identifier(@1,$1); }

PrintStmt     :   T_Print '(' ExprList ')' ';' { $$=new PrintStmt($3); }
              ;

ReturnStmt    :   T_Return OptionalExpr ';' { $$=new ReturnStmt(@2,$2); }
              ;

OptionalExpr  :   Expr                  { $$=$1; }
              |                         { $$=new EmptyExpr(); }
              ;

ExprList      :   ExprList ',' Expr     { ($$=$1)->Append($3); }
              |   Expr                  { ($$=new List<Expr*>)->Append($1); }
              ;

Expr          :   LValue '=' Expr       { Operator *op = new Operator(@2,"=");
                                          $$=new AssignExpr($1,op,$3); }
              |   Constant              { $$=$1; }
              |   LValue                { $$=$1; }
              |   T_This                { $$=new This(@1); }
              ; //TODO rest of expr

LValue        :   T_Identifier          { $$=new FieldAccess(NULL,new Identifier(@1,$1)); }
              |   Expr '.' T_Identifier { $$=new FieldAccess($1,new Identifier(@3,$3)); }
              |   Expr '[' Expr ']'     { $$=new ArrayAccess(@1,$1,$3); }
              ;

//Call          :   T_Identifier '(' 

Actuals       :   ExprList              { $$=$1; }
              |   /* empty string */    { $$=new List<Expr*>; }
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
