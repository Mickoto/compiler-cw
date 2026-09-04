parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program  : (class ';')+ ;

class    : CLASS classname=TYPEID (INHERITS inherit=TYPEID)? '{' (feature';')* '}' ;

feature  : attr | method ;

define   : OBJECTID ':' TYPEID;

formal   : define ;
method   : name=OBJECTID '(' (formal (',' formal)*)? ')' ':' TYPEID '{' body=expr '}' ;
attr     : define ('<-' expr)? ;

expr     : obj=expr ('@' TYPEID)? '.' name=OBJECTID '(' (args+=expr (',' args+=expr)* )? ')'
         | name=OBJECTID '(' (args+=expr (',' args+=expr)* )? ')'
         | if
         | while
         | let
         | block
         | case
         | unop=NEW TYPEID
         | unop='~' expr
         | unop=ISVOID expr
         | expr binop=('*' | '/') expr
         | expr binop=('+' | '-') expr
         | <assoc=right> expr binop=('<=' | '<' | '=') expr
         | unop=NOT expr
         | <assoc=right> OBJECTID '<-' expr
         | paren
         | object
         | integer
         | bool
         | string
         ;

if       : IF expr THEN expr ELSE expr FI ;
while    : WHILE expr LOOP expr POOL ;
block    : '{' (expr ';')+ '}' ;
letdef   : define ('<-' expr)? ;
let      : LET letdef (',' letdef)* IN expr ;
branch   : define '=>' expr ';' ;
case     : CASE expr OF branch+ ESAC ;
paren    : '(' expr ')' ;
object   : OBJECTID ;
integer  : INT_CONST ;
bool     : BOOL_CONST ;
string   : STR_CONST ;
