// Byte-exact VUC script_lex() wrapper; lexer class is a provisional ABI view.
class ScriptLexer {
public:
    virtual void action0();
    virtual void action1();
    virtual void action2();
    virtual void action3();
    virtual void action4();
    virtual void action5();
    virtual int yylex();
    int unused0;
    int unused1;
    int line;
};

extern ScriptLexer *lexer;
extern int script_line;

int script_lex()
{
    int token = lexer->yylex();
    script_line = lexer->line;
    return token;
}
