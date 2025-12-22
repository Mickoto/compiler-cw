lexer grammar CoolLexer;

// Тази част позволява дефинирането на допълнителен код, който да се запише в
// генерирания CoolLexer.h.
//
// Коментарите вътре са на английски, понеже ANTLR4 иначе ги омазва.

@lexer::header {
    #include<iomanip>
}

@lexer::members {
    // ----------------------- booleans -------------------------

    // A map from token ids to boolean values
    std::map<int, bool> bool_values;

    void assoc_bool_with_token(bool value) {
        bool_values[tokenStartCharIndex] = value;
    }

    bool get_bool_value(int token_start_char_index) {
        return bool_values.at(token_start_char_index);
    }

    // ----------------------- integers -------------------------

    // A map from token ids to integer values
    std::map<int, std::string> int_values;

    void assoc_int_with_token(std::string integer) {
        int_values[tokenStartCharIndex] = integer;
    }

    const std::string &get_int_value(int token_start_char_index) {
        return int_values.at(token_start_char_index);
    }

    // ----------------------- strings --------------------------

    // Maximum length of a constant string literal (CSL) excluding the implicit
    // null character that marks the end of the string.
    const unsigned MAX_STR_CONST = 1024;
    // Stores the current CSL as it's being built.
    std::vector<char> string_buffer;

    // A map from token ids to string values
    std::map<int, std::vector<char>> string_values;

    void add_char_to_current_string(char c) {
        string_buffer.push_back(c);
    }

    void add_escaped_char_to_current_string(char c) {
        switch (c) {
        case 'b':
            string_buffer.push_back('\b');
            break;
        case 't':
            string_buffer.push_back('\t');
            break;
        case 'n':
            string_buffer.push_back('\n');
            break;
        case 'f':
            string_buffer.push_back('\f');
            break;
        default:
            string_buffer.push_back(c);
        }
    }

    void clear_string_buffer() {
        string_buffer = std::vector<char>();
    }

    void assoc_current_string_with_token() {
        if (string_buffer.size() <= MAX_STR_CONST) {
            string_values[tokenStartCharIndex] = string_buffer;
        }
        else {
            setType(ERROR);
            assoc_message_with_error("String constant too long");
        }
        clear_string_buffer();
    }

    const std::string get_string_value(int token_start_char_index) {
        std::stringstream str;
        for (char c : string_values.at(token_start_char_index)) {
            str << c;
        }
        return str.str();
    }

    std::string pretty(char c) {
        switch (c) {
        case '\b':
            return "\\b";
        case '\t':
            return "\\t";
        case '\n':
            return "\\n";
        case '\f':
            return "\\f";
        case '"':
        case '\\':
            return std::string("\\") + c;
        default:
            if (c < 32) {
                std::stringstream str;
                str << "<0x" <<
                    std::setfill('0') << std::setw(2) << std::hex << (int)c << ">";
                return str.str();
            }
            else {
                return std::string() + c;
            }
        }
    }

    std::string get_string_pretty(int token_start_char_index) {
        std::stringstream str;
        for (char c : string_values.at(token_start_char_index)) {
            str << pretty(c);
        }
        return str.str();
    }

    // -------------------- object identifiers ------------------------

    // A map from token ids to object identifiers
    std::map<int, std::string> obj_identifiers;

    void assoc_typeid_with_token(std::string _typeid) {
        type_identifiers[tokenStartCharIndex] = _typeid;
    }

    const std::string &get_typeid_value(int token_start_char_index) {
        return type_identifiers.at(token_start_char_index);
    }

    // -------------------- type identifiers ------------------------

    // A map from token ids to type identifiers
    std::map<int, std::string> type_identifiers;

    void assoc_objid_with_token(std::string objid) {
        obj_identifiers[tokenStartCharIndex] = objid;
    }

    const std::string &get_objid_value(int token_start_char_index) {
        return obj_identifiers.at(token_start_char_index);
    }

    // ------------------- error messages -----------------------

    // A map from token ids to error messages
    std::map<int, std::string> error_messages;

    void assoc_message_with_error(std::string message) {
        error_messages[tokenStartCharIndex] = message;
    }

    const std::string &get_error_message(int token_start_char_index) {
        return error_messages.at(token_start_char_index);
    }
}

// --------------- прости жетони -------------------

SEMI   : ';';
LCURLY : '{';
RCURLY : '}';
LPARR : '(';
RPARR : ')';
COMMA : ',';
COLON : ':';
AT : '@';
DOT : '.';
PLUS : '+';
MINUS : '-';
TIMES : '*';
DIV : '/';
COMPL : '~';
LT : '<';
EQU : '=';
DARROW : '=>';
ASSIGN : '<-';
LE : '<=';

WS : [ \r\n\f\t\u000B] -> skip;

// -------------- помощни жетони -------------------

fragment A : [aA];
fragment B : [bB];
fragment C : [cC];
fragment D : [dD];
fragment E : [eE];
fragment F : [fF];
fragment G : [gG];
fragment H : [hH];
fragment I : [iI];
fragment J : [jJ];
fragment K : [kK];
fragment L : [lL];
fragment M : [mM];
fragment N : [nN];
fragment O : [oO];
fragment P : [pP];
fragment Q : [qQ];
fragment R : [rR];
fragment S : [sS];
fragment T : [tT];
fragment U : [uU];
fragment V : [vV];
fragment W : [wW];
fragment X : [xX];
fragment Y : [yY];
fragment Z : [zZ];

// --------------- булеви константи -------------------

BOOL_CONST : 't' R U E  { assoc_bool_with_token(true); }
           | 'f' A L S E { assoc_bool_with_token(false); };

// ---------------- целочислени константи -------------

INT_CONST : [0-9]+ { assoc_int_with_token(getText()); };

// --------------------- коментари --------------------

COMMENT_LINE : '--' .*? ('\n' | EOF) -> skip;
LCOMMENT : '(*' -> pushMode(COMMENT_MODE), skip;
ERROR_RCOMMENT : '*)'
    { assoc_message_with_error("Unmatched *)"); } -> type(ERROR);

mode COMMENT_MODE;
LCOMMENT2 : '(*' -> pushMode(COMMENT_MODE), skip;
RCOMMENT : '*)' -> popMode, skip;
ERROR_EOF_COMMENT : . EOF
    { assoc_message_with_error("EOF in comment"); } -> type(ERROR);

VALID_COMMENT : . -> skip;

// ----------------- ключови думи ---------------------

mode DEFAULT_MODE;
CLASS : C L A S S ;
ELSE : E L S E ;
FI : F I ;
IF : I F ;
IN : I N ;
INHERITS : I N H E R I T S ;
ISVOID : I S V O I D ;
LET : L E T ;
LOOP : L O O P ;
POOL : P O O L ;
THEN : T H E N ;
WHILE : W H I L E ;
CASE : C A S E ;
ESAC : E S A C ;
NEW : N E W ;
OF : O F ;
NOT : N O T ;

// 0--------------- текстови низове -------------------

OPEN_QUOTE : '"' -> skip, mode(STRING_MODE);

mode STRING_MODE;
STR_CONST : '"' { assoc_current_string_with_token(); } -> mode(DEFAULT_MODE);

ERROR_UNESCAPED_NEWLINE : '\n' {
    clear_string_buffer();
    assoc_message_with_error("String contains unescaped new line");
} -> type(ERROR), mode(DEFAULT_MODE);
ERROR_EOF_STRING : ~["\n] EOF
    { assoc_message_with_error("Unterminated string at EOF"); } -> type(ERROR);
ERROR_NULL_STRING : '\u0000' {
    clear_string_buffer();
    assoc_message_with_error("String contains null character");
} -> type(ERROR), mode(INVALID_STRING_MODE);
ERROR_ESCAPED_NULL_STRING : '\\\u0000' {
    clear_string_buffer();
    assoc_message_with_error("String contains escaped null character");
} -> type(ERROR), mode(INVALID_STRING_MODE);

ESCAPED_SYMBOL : '\\' .
    { add_escaped_char_to_current_string(getText()[1]); } -> skip;
VALID_LETTER : . { add_char_to_current_string(getText()[0]); } -> skip;

mode INVALID_STRING_MODE;
END_STRING : '"' -> skip, mode(DEFAULT_MODE);
SKIP_LETTER : . -> skip;

// ------------------ identifiers ---------------------

mode DEFAULT_MODE;
fragment ID_LETTER : [A-Za-z0-9_];
TYPEID : [A-Z] ID_LETTER* { assoc_typeid_with_token(getText()); };
OBJECTID : [a-z] ID_LETTER* { assoc_objid_with_token(getText()); };

// ----------------------- sink -----------------------

ERROR : . {{
    std::string message = "Invalid symbol \"";
    assoc_message_with_error(message + pretty(getText()[0]) + "\"");
}} ;
