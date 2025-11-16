parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program  : (class ';')+ ;

class    : CLASS classname=TYPEID (INHERITS inherit=TYPEID)? '{' (feature';')* '}' ;

feature  : attr
         | method
         ;

type     : TYPEID | SELF_TYPE ;
define   : OBJECTID ':' type;

formal   : define ;
method   : name=OBJECTID '(' (formal (',' formal)*)? ')' ':' type '{' body=expr '}' ;
attr     : define ('<-' expr)? ;

expr     : obj=expr ('@' type)? '.' name=OBJECTID '(' (args+=expr (',' args+=expr)* )? ')'
         | name=OBJECTID '(' (args+=expr (',' args+=expr)* )? ')'
         | if
         | while
         | let
         | block
         | case
         | NEW type
         | '~' expr
         | isvoid=ISVOID expr
         | expr op=('*' | '/') expr
         | expr op=('+' | '-') expr
         | <assoc=right> expr op=('<=' | '<' | '=') expr
         | NOT expr
         | <assoc=right> OBJECTID '<-' expr
         | paren
         | object
         | integer
         | bool
         | string
         ;

if       : IF expr THEN expr ELSE expr FI ;
while    : WHILE expr LOOP expr POOL ;
letdef   : define ('<-' expr)? ;
let      : LET letdef (',' letdef)* IN expr ;
// TODO: where block
block    : '{' (expr ';')+ '}' ;
branch   : define '=>' expr ';' ;
case     : CASE expr OF branch+ ESAC ;
paren    : '(' expr ')' ;
object   : OBJECTID | SELF ;
integer  : INT_CONST ;
bool     : BOOL_CONST ;
string   : STR_CONST ;
