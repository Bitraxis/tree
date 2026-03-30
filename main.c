#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "pkgtree_runtime.h"

#define MAX_TOKENS 4096
#define MAX_TEXT   256
#define MAX_VARS   256
#define MAX_ARRAY_ITEMS 1024

typedef enum {
    TOK_EOF = 0,
    TOK_IDENT,
    TOK_STRING,
    TOK_NUMBER,

    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACK, TOK_RBRACK,
    TOK_LBRACE, TOK_RBRACE,
    TOK_COMMA, TOK_DOT, TOK_SEMI,

    TOK_EQ,      /* = */
    TOK_EQEQ,    /* == */
    TOK_NEQ,     /* != */
    TOK_PLUS,    /* + */
    TOK_MINUS,   /* - */
    TOK_STAR,    /* * */
    TOK_SLASH,   /* / */
    TOK_LT,      /* < */
    TOK_LTE,     /* <= */
    TOK_GT,      /* > */
    TOK_GTE,     /* >= */

    TOK_COLON, TOK_DCOLON,

    TOK_NEWLINE,
    TOK_INDENT,
    TOK_DEDENT
} TokenKind;


/* ============ AST ============ */

typedef enum {
    AST_PROGRAM,
    AST_ASSIGN,
    AST_EXPR_STMT,
    AST_CALL,
    AST_BINARY,
    AST_IF,
    AST_WHILE,
    AST_BLOCK,
    AST_FUNC_DECL,
    AST_RETURN,
    AST_LIST,
    AST_DICT,
    AST_KV,
    AST_IDENT,
    AST_STRING,
    AST_NUMBER,
    AST_PKG
} AstNodeKind;




typedef struct AstNode AstNode;
struct AstNode {
    AstNodeKind kind;
    char text[MAX_TEXT];
    AstNode *children[32];
    int child_count;
    int line, col;
};

/* ============ Value Type ============ */

typedef enum {
    VAL_NIL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_LIST,
    VAL_DICT
} ValueKind;

typedef struct Value Value;
struct Value {
    ValueKind kind;
    union {
        long int_val;
        double float_val;
        char *str_val;
        struct {
            Value *items[MAX_ARRAY_ITEMS];
            int count;
        } list_val;
        struct {
            char *keys[MAX_ARRAY_ITEMS];
            Value *values[MAX_ARRAY_ITEMS];
            int count;
        } dict_val;
    } data;
};

/* ============ Symbol Table ============ */

typedef struct {
    char name[MAX_TEXT];
    Value val;
} Symbol;

typedef struct {
    Symbol items[MAX_VARS];
    int count;
} SymTable;


/* ============ Value Helpers ============ */

static Value val_nil(void) {
    Value v = {0};
    v.kind = VAL_NIL;
    return v;
}

static Value val_int(long i) {
    Value v = {0};
    v.kind = VAL_INT;
    v.data.int_val = i;
    return v;
}

static Value val_float(double d) {
    Value v = {0};
    v.kind = VAL_FLOAT;
    v.data.float_val = d;
    return v;
}

static Value val_string(const char *s) {
    Value v = {0};
    v.kind = VAL_STRING;
    v.data.str_val = (char *)malloc(strlen(s) + 1);
    strcpy(v.data.str_val, s);
    return v;
}

static Value val_list(void) {
    Value v = {0};
    v.kind = VAL_LIST;
    v.data.list_val.count = 0;
    return v;
}

static Value val_dict(void) {
    Value v = {0};
    v.kind = VAL_DICT;
    v.data.dict_val.count = 0;
    return v;
}

static void val_free(Value *v) {
    if (!v) return;
    if (v->kind == VAL_STRING && v->data.str_val) free(v->data.str_val);
    if (v->kind == VAL_LIST) {
        int i;
        for (i = 0; i < v->data.list_val.count; i++) {
            val_free(v->data.list_val.items[i]);
            free(v->data.list_val.items[i]);
        }
    }
    if (v->kind == VAL_DICT) {
        int i;
        for (i = 0; i < v->data.dict_val.count; i++) {
            if (v->data.dict_val.keys[i]) free(v->data.dict_val.keys[i]);
            val_free(v->data.dict_val.values[i]);
            free(v->data.dict_val.values[i]);
        }
    }
}

static char *val_to_string(const Value *v) {
    static char buf[MAX_TEXT];
    memset(buf, 0, sizeof(buf));

    switch (v->kind) {
        case VAL_NIL: strcpy(buf, "nil"); break;
        case VAL_INT: snprintf(buf, sizeof(buf), "%ld", v->data.int_val); break;
        case VAL_FLOAT: snprintf(buf, sizeof(buf), "%g", v->data.float_val); break;
        case VAL_STRING: strcpy(buf, v->data.str_val); break;
        case VAL_LIST: strcpy(buf, "[...]"); break;
        case VAL_DICT: strcpy(buf, "{...}"); break;
    }
    return buf;
}

/* ============ AST Helpers ============ */

static AstNode *ast_new(AstNodeKind kind, const char *text, int line, int col) {
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    n->kind = kind;
    n->line = line;
    n->col = col;
    if (text) strncpy(n->text, text, MAX_TEXT - 1);
    n->child_count = 0;
    return n;
}

static void ast_add_child(AstNode *parent, AstNode *child) {
    if (parent->child_count < 32) {
        parent->children[parent->child_count++] = child;
    }
}

/* ============ Symbol Table Helpers ============ */

static Value *sym_get(SymTable *table, const char *name) {
    int i;
    for (i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].name, name) == 0) {
            return &table->items[i].val;
        }
    }
    return NULL;
}

static void sym_set(SymTable *table, const char *name, Value val) {
    int i;
    for (i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].name, name) == 0) {
            val_free(&table->items[i].val);
            table->items[i].val = val;
            return;
        }
    }
    if (table->count < MAX_VARS) {
        strncpy(table->items[table->count].name, name, MAX_TEXT - 1);
        table->items[table->count].val = val;
        table->count++;
    }
}

typedef struct {
    char name[MAX_TEXT];
    char params[16][MAX_TEXT];
    int param_count;
    AstNode *body;
} FunctionDef;

typedef struct {
    FunctionDef items[128];
    int count;
} FunctionTable;

typedef struct {
    int has_return;
    Value return_value;
} ExecCtx;

static FunctionTable g_funcs = {0};


typedef struct {
    TokenKind kind;
    char text[MAX_TEXT];
    int line;
    int col;
} Token;

typedef struct {
    const char *src;
    size_t pos;
    int line;
    int col;
    Token tokens[MAX_TOKENS];
    int count;
} Lexer;

static void lex_emit(Lexer *lx, TokenKind kind, const char *text) {
    if (lx->count >= MAX_TOKENS) {
        fprintf(stderr, "Too many tokens\n");
        exit(1);
    }
    Token *t = &lx->tokens[lx->count++];
    t->kind = kind;
    strncpy(t->text, text ? text : "", MAX_TEXT - 1);
    t->text[MAX_TEXT - 1] = '\0';
    t->line = lx->line;
    t->col = lx->col;
}

static char lex_peek(Lexer *lx) { return lx->src[lx->pos]; }
static char lex_next(Lexer *lx) {
    char c = lx->src[lx->pos++];
    if (c == '\n') { lx->line++; lx->col = 1; }
    else lx->col++;
    return c;
}

static void lex_string(Lexer *lx) {
    char buf[MAX_TEXT] = {0};
    int i = 0;
    lex_next(lx); // opening "
    while (lex_peek(lx) && lex_peek(lx) != '"') {
        if (i < MAX_TEXT - 1) buf[i++] = lex_next(lx);
        else lex_next(lx);
    }
    if (lex_peek(lx) == '"') lex_next(lx);
    lex_emit(lx, TOK_STRING, buf);
}

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_' || c == '-';
}
static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.';
}


static void lex_ident(Lexer *lx) {
    char buf[MAX_TEXT] = {0};
    int i = 0;
    while (is_ident_char(lex_peek(lx))) {
        if (i < MAX_TEXT - 1) buf[i++] = lex_next(lx);
        else lex_next(lx);
    }
    lex_emit(lx, TOK_IDENT, buf);
}

static void lex_number(Lexer *lx) {
    char buf[MAX_TEXT] = {0};
    int i = 0;
    while (isdigit((unsigned char)lex_peek(lx)) || lex_peek(lx) == '.') {
        if (i < MAX_TEXT - 1) buf[i++] = lex_next(lx);
        else lex_next(lx);
    }
    lex_emit(lx, TOK_NUMBER, buf);
}

static void lex_all(Lexer *lx) {
    while (lex_peek(lx)) {
        char c = lex_peek(lx);

        /* whitespace */
        if (c == ' ' || c == '\t' || c == '\r') {
            lex_next(lx);
            continue;
        }

        /* newline */
        if (c == '\n') {
            lex_next(lx);
            lex_emit(lx, TOK_NEWLINE, "\\n");
            continue;
        }

        /* line comment: # ... (to end of line, newline token still emitted later) */
        if (c == '#') {
            while (lex_peek(lx) && lex_peek(lx) != '\n') {
                lex_next(lx);
            }
            continue;
        }

        /* block comment: //# ... #// */
        if (c == '/' &&
            lx->src[lx->pos + 1] == '/' &&
            lx->src[lx->pos + 2] == '#') {

            /* consume opening '//#' */
            lex_next(lx); /* / */
            lex_next(lx); /* / */
            lex_next(lx); /* # */

            /* consume until closing '#//' */
            while (lex_peek(lx)) {
                if (lex_peek(lx) == '#' &&
                    lx->src[lx->pos + 1] == '/' &&
                    lx->src[lx->pos + 2] == '/') {
                    lex_next(lx); /* c# */
                    lex_next(lx); /* / */
                    lex_next(lx); /* / */
                    break;
                }
                lex_next(lx);
            }

            /* optional: error if unterminated block comment */
            /* if (!lex_peek(lx)) {
                fprintf(stderr, "Unterminated block comment at %d:%d\n", lx->line, lx->col);
                exit(1);
            } */

            continue;
        }

        if (c == '"') { lex_string(lx); continue; }
        if (is_ident_start(c)) { lex_ident(lx); continue; }
        if (isdigit((unsigned char)c)) { lex_number(lx); continue; }

        switch (c) {
            case '(': lex_next(lx); lex_emit(lx, TOK_LPAREN, "("); break;
            case ')': lex_next(lx); lex_emit(lx, TOK_RPAREN, ")"); break;
            case '[': lex_next(lx); lex_emit(lx, TOK_LBRACK, "["); break;
            case ']': lex_next(lx); lex_emit(lx, TOK_RBRACK, "]"); break;
            case '{': lex_next(lx); lex_emit(lx, TOK_LBRACE, "{"); break;
            case '}': lex_next(lx); lex_emit(lx, TOK_RBRACE, "}"); break;
            case ',': lex_next(lx); lex_emit(lx, TOK_COMMA, ","); break;
            case ';': lex_next(lx); lex_emit(lx, TOK_SEMI, ";"); break;
            case '.': lex_next(lx); lex_emit(lx, TOK_DOT, "."); break;

            case '+': lex_next(lx); lex_emit(lx, TOK_PLUS, "+"); break;
            case '-': lex_next(lx); lex_emit(lx, TOK_MINUS, "-"); break;
            case '*': lex_next(lx); lex_emit(lx, TOK_STAR, "*"); break;

            case '/':
                lex_next(lx);
                lex_emit(lx, TOK_SLASH, "/");
                break;

            case '=':
                lex_next(lx);
                if (lex_peek(lx) == '=') {
                    lex_next(lx);
                    lex_emit(lx, TOK_EQEQ, "==");
                } else {
                    lex_emit(lx, TOK_EQ, "=");
                }
                break;

            case '!':
                lex_next(lx);
                if (lex_peek(lx) == '=') {
                    lex_next(lx);
                    lex_emit(lx, TOK_NEQ, "!=");
                } else {
                    fprintf(stderr, "Unknown character '!' at %d:%d\n", lx->line, lx->col);
                    exit(1);
                }
                break;

            case '<':
                lex_next(lx);
                if (lex_peek(lx) == '=') { lex_next(lx); lex_emit(lx, TOK_LTE, "<="); }
                else lex_emit(lx, TOK_LT, "<");
                break;

            case '>':
                lex_next(lx);
                if (lex_peek(lx) == '=') { lex_next(lx); lex_emit(lx, TOK_GTE, ">="); }
                else lex_emit(lx, TOK_GT, ">");
                break;

            case ':':
                lex_next(lx);
                if (lex_peek(lx) == ':') { lex_next(lx); lex_emit(lx, TOK_DCOLON, "::"); }
                else lex_emit(lx, TOK_COLON, ":");
                break;

            default:
                fprintf(stderr, "Unknown character '%c' at %d:%d\n", c, lx->line, lx->col);
                exit(1);
        }

    }

    lex_emit(lx, TOK_EOF, "EOF");
}


typedef struct {
    Token *tokens;
    int pos;
    int count;
} Parser;

static Token *p_peek(Parser *p) { return &p->tokens[p->pos]; }
static Token *p_next(Parser *p) { return &p->tokens[p->pos++]; }

static int p_match(Parser *p, TokenKind k) {
    if (p_peek(p)->kind == k) { p_next(p); return 1; }
    return 0;
}

static Token *p_expect(Parser *p, TokenKind k, const char *msg) {
    if (p_peek(p)->kind != k) {
        Token *t = p_peek(p);
        fprintf(stderr, "Parse error at %d:%d: expected %s, got '%s'\n",
                t->line, t->col, msg, t->text);
        exit(1);
    }
    return p_next(p);
}

/* Forward decls */
static AstNode *parse_expr(Parser *p);
static AstNode *parse_func_decl(Parser *p);
static AstNode *parse_return_stmt(Parser *p);
static int is_func_decl_start(Parser *p);
static AstNode *parse_stmt(Parser *p);
static AstNode *parse_block(Parser *p);
static AstNode *parse_if_stmt(Parser *p);
static AstNode *parse_while_stmt(Parser *p);


static AstNode *parse_list(Parser *p) {
    Token *start = p_expect(p, TOK_LBRACK, "[");
    AstNode *list = ast_new(AST_LIST, "", start->line, start->col);

    if (!p_match(p, TOK_RBRACK)) {
        while (1) {
            if (p_peek(p)->kind == TOK_IDENT &&
                p->pos + 1 < p->count &&
                p->tokens[p->pos + 1].kind == TOK_EQ) {
                Token *key = p_expect(p, TOK_IDENT, "key");
                p_expect(p, TOK_EQ, "=");
                AstNode *kv = ast_new(AST_KV, key->text, key->line, key->col);
                ast_add_child(kv, parse_expr(p));
                ast_add_child(list, kv);
            } else {
                ast_add_child(list, parse_expr(p));
            }
            if (!p_match(p, TOK_COMMA)) break;
        }
        p_expect(p, TOK_RBRACK, "]");
    }
    return list;
}

static AstNode *parse_dict(Parser *p) {
    Token *start = p_expect(p, TOK_LBRACE, "{");
    AstNode *dict = ast_new(AST_DICT, "", start->line, start->col);

    if (!p_match(p, TOK_RBRACE)) {
        while (1) {
            Token *key = p_expect(p, TOK_IDENT, "key");
            p_expect(p, TOK_EQ, "=");
            AstNode *kv = ast_new(AST_KV, key->text, key->line, key->col);
            ast_add_child(kv, parse_expr(p));
            ast_add_child(dict, kv);
            if (!p_match(p, TOK_COMMA)) break;
        }
        p_expect(p, TOK_RBRACE, "}");
    }
    return dict;
}

static AstNode *parse_primary(Parser *p) {
    Token *t = p_peek(p);

    if (t->kind == TOK_IDENT) {
        Token *name = p_next(p);

        /* namespace call: ns::fn(...) */
        if (p_match(p, TOK_DCOLON)) {
            Token *rhs = p_expect(p, TOK_IDENT, "identifier");
            char merged[MAX_TEXT];
            snprintf(merged, sizeof(merged), "%s::%s", name->text, rhs->text);

            if (p_match(p, TOK_LPAREN)) {
                AstNode *call = ast_new(AST_CALL, merged, name->line, name->col);
                if (!p_match(p, TOK_RPAREN)) {
                    ast_add_child(call, parse_expr(p));
                    while (p_match(p, TOK_COMMA)) ast_add_child(call, parse_expr(p));
                    p_expect(p, TOK_RPAREN, ")");
                }
                return call;
            }
            return ast_new(AST_IDENT, merged, name->line, name->col);
        }

        /* comma-family name: To,Sub(...) */
        if (p_match(p, TOK_COMMA)) {
            Token *rhs = p_expect(p, TOK_IDENT, "identifier");
            char merged[MAX_TEXT];
            snprintf(merged, sizeof(merged), "%s,%s", name->text, rhs->text);

            if (p_match(p, TOK_LPAREN)) {
                AstNode *call = ast_new(AST_CALL, merged, name->line, name->col);
                if (!p_match(p, TOK_RPAREN)) {
                    ast_add_child(call, parse_expr(p));
                    while (p_match(p, TOK_COMMA)) ast_add_child(call, parse_expr(p));
                    p_expect(p, TOK_RPAREN, ")");
                }
                return call;
            }
            return ast_new(AST_IDENT, merged, name->line, name->col);
        }

        /* simple call/name */
        if (p_match(p, TOK_LPAREN)) {
            AstNode *call = ast_new(AST_CALL, name->text, name->line, name->col);
            if (!p_match(p, TOK_RPAREN)) {
                ast_add_child(call, parse_expr(p));
                while (p_match(p, TOK_COMMA)) ast_add_child(call, parse_expr(p));
                p_expect(p, TOK_RPAREN, ")");
            }
            return call;
        }

        return ast_new(AST_IDENT, name->text, name->line, name->col);
    }


    if (t->kind == TOK_STRING) { p_next(p); return ast_new(AST_STRING, t->text, t->line, t->col); }
    if (t->kind == TOK_NUMBER) { p_next(p); return ast_new(AST_NUMBER, t->text, t->line, t->col); }
    if (t->kind == TOK_LBRACK) return parse_list(p);
    if (t->kind == TOK_LBRACE) return parse_dict(p);

    if (p_match(p, TOK_LPAREN)) {
        AstNode *inner = parse_expr(p);
        p_expect(p, TOK_RPAREN, ")");
        return inner;
    }

    fprintf(stderr, "Parse error at %d:%d: unexpected token '%s'\n", t->line, t->col, t->text);
    exit(1);
}

static AstNode *parse_mul(Parser *p) {
    AstNode *left = parse_primary(p);
    while (p_peek(p)->kind == TOK_STAR || p_peek(p)->kind == TOK_SLASH) {
        Token *op = p_next(p);
        AstNode *right = parse_primary(p);
        AstNode *bin = ast_new(AST_BINARY, op->text, op->line, op->col);
        ast_add_child(bin, left);
        ast_add_child(bin, right);
        left = bin;
    }
    return left;
}

static AstNode *parse_add(Parser *p) {
    AstNode *left = parse_mul(p);
    while (p_peek(p)->kind == TOK_PLUS || p_peek(p)->kind == TOK_MINUS) {
        Token *op = p_next(p);
        AstNode *right = parse_mul(p);
        AstNode *bin = ast_new(AST_BINARY, op->text, op->line, op->col);
        ast_add_child(bin, left);
        ast_add_child(bin, right);
        left = bin;
    }
    return left;
}

static AstNode *parse_cmp(Parser *p) {
    AstNode *left = parse_add(p);
    while (p_peek(p)->kind == TOK_LT || p_peek(p)->kind == TOK_LTE ||
           p_peek(p)->kind == TOK_GT || p_peek(p)->kind == TOK_GTE) {
        Token *op = p_next(p);
        AstNode *right = parse_add(p);
        AstNode *bin = ast_new(AST_BINARY, op->text, op->line, op->col);
        ast_add_child(bin, left);
        ast_add_child(bin, right);
        left = bin;
    }
    return left;
}

static AstNode *parse_eq_expr(Parser *p) {
    AstNode *left = parse_cmp(p);
    while (p_peek(p)->kind == TOK_EQEQ || p_peek(p)->kind == TOK_NEQ) {
        Token *op = p_next(p);
        AstNode *right = parse_cmp(p);
        AstNode *bin = ast_new(AST_BINARY, op->text, op->line, op->col);
        ast_add_child(bin, left);
        ast_add_child(bin, right);
        left = bin;
    }
    return left;
}

static AstNode *parse_expr(Parser *p) {
    return parse_eq_expr(p);
}

static AstNode *parse_block(Parser *p) {
    Token *start = p_expect(p, TOK_LBRACE, "{");
    AstNode *block = ast_new(AST_BLOCK, "", start->line, start->col);

    while (p_match(p, TOK_NEWLINE)) {}
    while (!p_match(p, TOK_RBRACE)) {
        ast_add_child(block, parse_stmt(p));
        while (p_match(p, TOK_NEWLINE)) {}
    }
    return block;
}

static AstNode *parse_if_stmt(Parser *p) {
    Token *kw = p_expect(p, TOK_IDENT, "if");
    if (strcmp(kw->text, "if") != 0) {
        fprintf(stderr, "Expected 'if' keyword\n");
        exit(1);
    }

    p_expect(p, TOK_LPAREN, "(");
    AstNode *cond = parse_expr(p);
    p_expect(p, TOK_RPAREN, ")");
    AstNode *block = parse_block(p);

    AstNode *n = ast_new(AST_IF, "if", kw->line, kw->col);
    ast_add_child(n, cond);
    ast_add_child(n, block);
    p_expect(p, TOK_SEMI, ";");
    return n;
}

static AstNode *parse_while_stmt(Parser *p) {
    Token *kw = p_expect(p, TOK_IDENT, "while");
    if (strcmp(kw->text, "while") != 0) {
        fprintf(stderr, "Expected 'while' keyword\n");
        exit(1);
    }

    p_expect(p, TOK_LPAREN, "(");
    AstNode *cond = parse_expr(p);
    p_expect(p, TOK_RPAREN, ")");
    AstNode *block = parse_block(p);

    AstNode *n = ast_new(AST_WHILE, "while", kw->line, kw->col);
    ast_add_child(n, cond);
    ast_add_child(n, block);
    p_expect(p, TOK_SEMI, ";");
    return n;
}

static int is_func_decl_start(Parser *p) {
    int i = p->pos;

    if (i >= p->count || p->tokens[i].kind != TOK_IDENT) return 0;

    /* optional namespace prefix */
    if (i + 2 < p->count &&
        p->tokens[i + 1].kind == TOK_DCOLON &&
        p->tokens[i + 2].kind == TOK_IDENT) {
        i += 3;
    } else {
        i += 1;
    }

    if (i >= p->count || p->tokens[i].kind != TOK_LPAREN) return 0;
    i++;

    int depth = 1;
    while (i < p->count && depth > 0) {
        if (p->tokens[i].kind == TOK_LPAREN) depth++;
        else if (p->tokens[i].kind == TOK_RPAREN) depth--;
        i++;
    }
    if (depth != 0) return 0;
    if (i >= p->count) return 0;

    return p->tokens[i].kind == TOK_LBRACE;
}


static AstNode *parse_func_decl(Parser *p) {
    Token *first = p_expect(p, TOK_IDENT, "function name");
    char func_name[MAX_TEXT] = {0};
    strncpy(func_name, first->text, MAX_TEXT - 1);

    if (p_match(p, TOK_DCOLON)) {
        Token *rhs = p_expect(p, TOK_IDENT, "function name");
        snprintf(func_name, sizeof(func_name), "%s::%s", first->text, rhs->text);
    }

    AstNode *fn = ast_new(AST_FUNC_DECL, func_name, first->line, first->col);

    p_expect(p, TOK_LPAREN, "(");
    if (!p_match(p, TOK_RPAREN)) {
        while (1) {
            Token *param = p_expect(p, TOK_IDENT, "parameter");

            char pname[MAX_TEXT] = {0};
            strncpy(pname, param->text, MAX_TEXT - 1);

            /* support lexer behavior where comma can be part of ident */
            size_t n = strlen(pname);
            if (n > 0 && pname[n - 1] == ',') {
                pname[n - 1] = '\0';
                AstNode *param_node = ast_new(AST_IDENT, pname, param->line, param->col);
                ast_add_child(fn, param_node);
                continue; /* comma already consumed as part of token */
            }

            AstNode *param_node = ast_new(AST_IDENT, pname, param->line, param->col);
            ast_add_child(fn, param_node);

            if (!p_match(p, TOK_COMMA)) break;
        }
        p_expect(p, TOK_RPAREN, ")");
    }


    AstNode *body = parse_block(p);
    ast_add_child(fn, body);

    p_expect(p, TOK_SEMI, ";");
    return fn;
}


static AstNode *parse_return_stmt(Parser *p) {
    Token *kw = p_expect(p, TOK_IDENT, "return");
    if (strcmp(kw->text, "return") != 0) {
        fprintf(stderr, "Expected 'return' keyword\n");
        exit(1);
    }

    AstNode *ret = ast_new(AST_RETURN, "return", kw->line, kw->col);
    ast_add_child(ret, parse_expr(p));
    p_expect(p, TOK_SEMI, ";");
    return ret;
}


static AstNode *parse_stmt(Parser *p) {
    if (p_peek(p)->kind == TOK_IDENT && strcmp(p_peek(p)->text, "if") == 0) {
        return parse_if_stmt(p);
    }

    if (p_peek(p)->kind == TOK_IDENT && strcmp(p_peek(p)->text, "while") == 0) {
        return parse_while_stmt(p);
    }

    if (p_peek(p)->kind == TOK_IDENT && strcmp(p_peek(p)->text, "return") == 0) {
        return parse_return_stmt(p);
    }

    if (is_func_decl_start(p)) {
        return parse_func_decl(p);
    }

    /* assignment: ident = expr; OR ns::name = expr; */
    if (p_peek(p)->kind == TOK_IDENT) {
        if (p->pos + 1 < p->count && p->tokens[p->pos + 1].kind == TOK_EQ) {
            Token *lhs = p_expect(p, TOK_IDENT, "identifier");
            p_expect(p, TOK_EQ, "=");
            AstNode *assign = ast_new(AST_ASSIGN, lhs->text, lhs->line, lhs->col);
            ast_add_child(assign, parse_expr(p));
            p_expect(p, TOK_SEMI, ";");
            return assign;
        }

        if (p->pos + 3 < p->count &&
            p->tokens[p->pos + 1].kind == TOK_DCOLON &&
            p->tokens[p->pos + 2].kind == TOK_IDENT &&
            p->tokens[p->pos + 3].kind == TOK_EQ) {

            Token *ns = p_expect(p, TOK_IDENT, "namespace");
            p_expect(p, TOK_DCOLON, "::");
            Token *name = p_expect(p, TOK_IDENT, "identifier");
            p_expect(p, TOK_EQ, "=");

            char lhs_name[MAX_TEXT];
            snprintf(lhs_name, sizeof(lhs_name), "%s::%s", ns->text, name->text);

            AstNode *assign = ast_new(AST_ASSIGN, lhs_name, ns->line, ns->col);
            ast_add_child(assign, parse_expr(p));
            p_expect(p, TOK_SEMI, ";");
            return assign;
        }
    }

    AstNode *expr_stmt = ast_new(AST_EXPR_STMT, "", p_peek(p)->line, p_peek(p)->col);
    ast_add_child(expr_stmt, parse_expr(p));
    p_expect(p, TOK_SEMI, ";");
    return expr_stmt;
}




static AstNode *parse_pkg_stmt(Parser *p) {
    Token *pkg_kw = p_expect(p, TOK_IDENT, "pkg");
    if (strcmp(pkg_kw->text, "pkg") != 0) {
        fprintf(stderr, "Expected 'pkg' keyword\n");
        exit(1);
    }

    p_expect(p, TOK_LPAREN, "(");
    p_expect(p, TOK_RPAREN, ")");
    p_expect(p, TOK_LBRACE, "{");

    AstNode *pkg_node = ast_new(AST_PKG, "", pkg_kw->line, pkg_kw->col);

    while (!p_match(p, TOK_RBRACE)) {
        if (p_match(p, TOK_NEWLINE)) continue;

        Token *pkg_str = p_expect(p, TOK_STRING, "package name");
        AstNode *pkg_ref = ast_new(AST_STRING, pkg_str->text, pkg_str->line, pkg_str->col);
        ast_add_child(pkg_node, pkg_ref);

        p_expect(p, TOK_SEMI, ";");
        while (p_match(p, TOK_NEWLINE)) {}
    }

    char namespace_alias[MAX_TEXT] = {0};
    if (p_peek(p)->kind == TOK_IDENT && strcmp(p_peek(p)->text, "as") == 0) {
        p_next(p);
        Token *alias = p_expect(p, TOK_IDENT, "alias");
        strncpy(namespace_alias, alias->text, MAX_TEXT - 1);
    }

    if (strlen(namespace_alias) > 0) {
        strncpy(pkg_node->text, namespace_alias, MAX_TEXT - 1);
    }

    p_expect(p, TOK_SEMI, ";");
    return pkg_node;
}

static AstNode *parse_program(Parser *p) {
    AstNode *prog = ast_new(AST_PROGRAM, "", 1, 1);

    while (p_peek(p)->kind != TOK_EOF) {
        if (p_match(p, TOK_NEWLINE)) continue;
        if (p_peek(p)->kind == TOK_IDENT && strcmp(p_peek(p)->text, "pkg") == 0) {
            ast_add_child(prog, parse_pkg_stmt(p));
        } else {
            ast_add_child(prog, parse_stmt(p));
        }
        while (p_match(p, TOK_NEWLINE)) {}
    }

    return prog;
}


/* ============ Interpreter ============ */


static double val_to_num(const Value *v) {
    if (v->kind == VAL_INT) return (double)v->data.int_val;
    if (v->kind == VAL_FLOAT) return v->data.float_val;
    if (v->kind == VAL_STRING) return strtod(v->data.str_val, NULL);
    return 0.0;
}

static Value eval_expr_ctx(AstNode *node, SymTable *table, ExecCtx *ctx);
static void eval_stmt_ctx(AstNode *node, SymTable *table, ExecCtx *ctx);


static Value eval_expr(AstNode *node, SymTable *table) {
    return eval_expr_ctx(node, table, NULL);
}

static void eval_stmt(AstNode *node, SymTable *table) {
    eval_stmt_ctx(node, table, NULL);
}

static void register_function(AstNode *fn);
static FunctionDef *find_function(const char *name);




static Value builtin_print(AstNode **args, int argc, SymTable *table) {
    int i;
    for (i = 0; i < argc; i++) {
        Value v = eval_expr(args[i], table);
        printf("%s", val_to_string(&v));
        val_free(&v);
    }
    printf("\n");
    return val_nil();
}

static Value builtin_to_int(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) {
        fprintf(stderr, "to.Int or To,Int expects 1 argument\n");
        return val_nil();
    }
    Value v = eval_expr(args[0], table);
    long result = 0;

    switch (v.kind) {
        case VAL_INT: result = v.data.int_val; break;
        case VAL_FLOAT: result = (long)v.data.float_val; break;
        case VAL_STRING: result = strtol(v.data.str_val, NULL, 10); break;
        default: result = 0;
    }

    val_free(&v);
    return val_int(result);
}

static Value builtin_to_float(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) {
        fprintf(stderr, "to_float expects 1 argument\n");
        return val_nil();
    }
    Value v = eval_expr(args[0], table);
    double result = 0.0;

    switch (v.kind) {
        case VAL_INT: result = (double)v.data.int_val; break;
        case VAL_FLOAT: result = v.data.float_val; break;
        case VAL_STRING: result = strtod(v.data.str_val, NULL); break;
        default: result = 0.0;
    }

    val_free(&v);
    return val_float(result);
}

static Value builtin_to_string(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) {
        fprintf(stderr, "to.String expects 1 argument\n");
        return val_nil();
    }
    Value v = eval_expr(args[0], table);
    char *result = val_to_string(&v);
    Value str_val = val_string(result);
    val_free(&v);
    return str_val;
}

/* ============ Math Functions (Float Family) ============ */

static Value builtin_to_sqrt(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to_sqrt expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    double d = 0.0;
    if (v.kind == VAL_FLOAT) d = v.data.float_val;
    else if (v.kind == VAL_INT) d = (double)v.data.int_val;
    val_free(&v);
    return val_float(sqrt(d));
}

static Value builtin_to_pow(AstNode **args, int argc, SymTable *table) {
    if (argc != 2) { fprintf(stderr, "to_pow expects 2 arguments\n"); return val_nil(); }
    Value base = eval_expr(args[0], table);
    Value exp = eval_expr(args[1], table);
    double b = (base.kind == VAL_FLOAT) ? base.data.float_val : (double)base.data.int_val;
    double e = (exp.kind == VAL_FLOAT) ? exp.data.float_val : (double)exp.data.int_val;
    val_free(&base);
    val_free(&exp);
    return val_float(pow(b, e));
}

static Value builtin_to_abs(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to_abs expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    if (v.kind == VAL_FLOAT) { double d = fabs(v.data.float_val); val_free(&v); return val_float(d); }
    if (v.kind == VAL_INT) { long i = labs(v.data.int_val); val_free(&v); return val_int(i); }
    val_free(&v);
    return val_nil();
}

static Value builtin_to_sin(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to_sin expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    double d = (v.kind == VAL_FLOAT) ? v.data.float_val : (double)v.data.int_val;
    val_free(&v);
    return val_float(sin(d));
}

static Value builtin_to_cos(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to_cos expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    double d = (v.kind == VAL_FLOAT) ? v.data.float_val : (double)v.data.int_val;
    val_free(&v);
    return val_float(cos(d));
}

static Value builtin_to_floor(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to_floor expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    double d = (v.kind == VAL_FLOAT) ? v.data.float_val : (double)v.data.int_val;
    val_free(&v);
    return val_float(floor(d));
}

static Value builtin_to_ceil(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to_ceil expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    double d = (v.kind == VAL_FLOAT) ? v.data.float_val : (double)v.data.int_val;
    val_free(&v);
    return val_float(ceil(d));
}

/* ============ Integer Math Functions (DEC: Alternative Integer Ops) ============ */

static Value builtin_to_add(AstNode **args, int argc, SymTable *table) {
    if (argc != 2) { fprintf(stderr, "To,Add expects 2 arguments\n"); return val_nil(); }
    Value a = eval_expr(args[0], table);
    Value b = eval_expr(args[1], table);
    long result = ((a.kind == VAL_INT) ? a.data.int_val : (long)a.data.float_val) +
                  ((b.kind == VAL_INT) ? b.data.int_val : (long)b.data.float_val);
    val_free(&a);
    val_free(&b);
    return val_int(result);
}

static Value builtin_to_sub(AstNode **args, int argc, SymTable *table) {
    if (argc != 2) { fprintf(stderr, "To,Sub expects 2 arguments\n"); return val_nil(); }
    Value a = eval_expr(args[0], table);
    Value b = eval_expr(args[1], table);
    long result = ((a.kind == VAL_INT) ? a.data.int_val : (long)a.data.float_val) -
                  ((b.kind == VAL_INT) ? b.data.int_val : (long)b.data.float_val);
    val_free(&a);
    val_free(&b);
    return val_int(result);
}

static Value builtin_to_mul(AstNode **args, int argc, SymTable *table) {
    if (argc != 2) { fprintf(stderr, "To,Mul expects 2 arguments\n"); return val_nil(); }
    Value a = eval_expr(args[0], table);
    Value b = eval_expr(args[1], table);
    long result = ((a.kind == VAL_INT) ? a.data.int_val : (long)a.data.float_val) *
                  ((b.kind == VAL_INT) ? b.data.int_val : (long)b.data.float_val);
    val_free(&a);
    val_free(&b);
    return val_int(result);
}

static Value builtin_to_div(AstNode **args, int argc, SymTable *table) {
    if (argc != 2) { fprintf(stderr, "To,Div expects 2 arguments\n"); return val_nil(); }
    Value a = eval_expr(args[0], table);
    Value b = eval_expr(args[1], table);
    long b_val = (b.kind == VAL_INT) ? b.data.int_val : (long)b.data.float_val;
    if (b_val == 0) { fprintf(stderr, "Division by zero\n"); val_free(&a); val_free(&b); return val_nil(); }
    long result = ((a.kind == VAL_INT) ? a.data.int_val : (long)a.data.float_val) / b_val;
    val_free(&a);
    val_free(&b);
    return val_int(result);
}

/* ============ String Functions (Dot Family) ============ */

static Value builtin_to_length(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to.Length expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    long len = 0;
    if (v.kind == VAL_STRING) len = (long)strlen(v.data.str_val);
    if (v.kind == VAL_LIST) len = (long)v.data.list_val.count;
    if (v.kind == VAL_DICT) len = (long)v.data.dict_val.count;
    val_free(&v);
    return val_int(len);
}

static Value builtin_to_upper(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to.Upper expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    if (v.kind != VAL_STRING) { val_free(&v); return val_nil(); }
    char buf[MAX_TEXT];
    int i = 0;
    for (i = 0; i < MAX_TEXT - 1 && v.data.str_val[i]; i++) {
        buf[i] = toupper((unsigned char)v.data.str_val[i]);
    }
    buf[i] = '\0';
    val_free(&v);
    return val_string(buf);
}

static Value builtin_to_lower(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "to.Lower expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    if (v.kind != VAL_STRING) { val_free(&v); return val_nil(); }
    char buf[MAX_TEXT];
    int i = 0;
    for (i = 0; i < MAX_TEXT - 1 && v.data.str_val[i]; i++) {
        buf[i] = tolower((unsigned char)v.data.str_val[i]);
    }
    buf[i] = '\0';
    val_free(&v);
    return val_string(buf);
}

/* ============ List Operations (Kebab Family) ============ */

static Value builtin_get_length(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "get-length expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    long len = 0;
    if (v.kind == VAL_LIST) len = (long)v.data.list_val.count;
    if (v.kind == VAL_DICT) len = (long)v.data.dict_val.count;
    val_free(&v);
    return val_int(len);
}

static Value builtin_get_first(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "get-first expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    if (v.kind != VAL_LIST || v.data.list_val.count == 0) return val_nil();
    Value result = *v.data.list_val.items[0];
    val_free(&v);
    return result;
}

static Value builtin_get_last(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "get-last expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    if (v.kind != VAL_LIST || v.data.list_val.count == 0) return val_nil();
    Value result = *v.data.list_val.items[v.data.list_val.count - 1];
    val_free(&v);
    return result;
}

static Value builtin_get_type(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "get-type expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    const char *type = "nil";
    switch (v.kind) {
        case VAL_NIL: type = "nil"; break;
        case VAL_INT: type = "int"; break;
        case VAL_FLOAT: type = "float"; break;
        case VAL_STRING: type = "string"; break;
        case VAL_LIST: type = "list"; break;
        case VAL_DICT: type = "dict"; break;
    }
    val_free(&v);
    return val_string(type);
}

/* ============ Random Package Functions ============ */

static Value builtin_random_rand(AstNode **args, int argc, SymTable *table) {
    if (argc != 0) { fprintf(stderr, "random::rand expects 0 arguments\n"); return val_nil(); }
    return val_float((double)rand() / RAND_MAX);
}

static Value builtin_random_rand_int(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "random::rand-int expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    long max = (v.kind == VAL_INT) ? v.data.int_val : (long)v.data.float_val;
    val_free(&v);
    if (max <= 0) return val_int(0);
    return val_int(rand() % max);
}

static Value builtin_random_rand_seed(AstNode **args, int argc, SymTable *table) {
    if (argc != 1) { fprintf(stderr, "random::rand-seed expects 1 argument\n"); return val_nil(); }
    Value v = eval_expr(args[0], table);
    unsigned int seed = (unsigned int)((v.kind == VAL_INT) ? v.data.int_val : (long)v.data.float_val);
    val_free(&v);
    srand(seed);
    return val_nil();
}

static Value eval_call(AstNode *node, SymTable *table) {
    const char *name = node->text;
    AstNode *args[32];
    int argc = 0;
    int i;

    for (i = 0; i < node->child_count; i++) {
        args[i] = node->children[i];
        argc++;
    }

    /* builtins (your existing builtin checks stay here unchanged) */
    if (strcmp(name, "print") == 0) return builtin_print(args, argc, table);
    if (strcmp(name, "to.String") == 0) return builtin_to_string(args, argc, table);
    if (strcmp(name, "To,Int") == 0) return builtin_to_int(args, argc, table);
    if (strcmp(name, "to_float") == 0) return builtin_to_float(args, argc, table);
    if (strcmp(name, "to_sqrt") == 0) return builtin_to_sqrt(args, argc, table);
    if (strcmp(name, "to_pow") == 0) return builtin_to_pow(args, argc, table);
    if (strcmp(name, "to_abs") == 0) return builtin_to_abs(args, argc, table);
    if (strcmp(name, "to_sin") == 0) return builtin_to_sin(args, argc, table);
    if (strcmp(name, "to_cos") == 0) return builtin_to_cos(args, argc, table);
    if (strcmp(name, "to_floor") == 0) return builtin_to_floor(args, argc, table);
    if (strcmp(name, "to_ceil") == 0) return builtin_to_ceil(args, argc, table);
    if (strcmp(name, "To,Add") == 0) return builtin_to_add(args, argc, table);
    if (strcmp(name, "To,Sub") == 0) return builtin_to_sub(args, argc, table);
    if (strcmp(name, "To,Mul") == 0) return builtin_to_mul(args, argc, table);
    if (strcmp(name, "To,Div") == 0) return builtin_to_div(args, argc, table);
    if (strcmp(name, "to.Length") == 0) return builtin_to_length(args, argc, table);
    if (strcmp(name, "to.Upper") == 0) return builtin_to_upper(args, argc, table);
    if (strcmp(name, "to.Lower") == 0) return builtin_to_lower(args, argc, table);
    if (strcmp(name, "get-length") == 0) return builtin_get_length(args, argc, table);
    if (strcmp(name, "get-first") == 0) return builtin_get_first(args, argc, table);
    if (strcmp(name, "get-last") == 0) return builtin_get_last(args, argc, table);
    if (strcmp(name, "get-type") == 0) return builtin_get_type(args, argc, table);

    /* user-defined */
    FunctionDef *fn = find_function(name);
    if (fn) {
        if (argc != fn->param_count) {
            fprintf(stderr, "%s expects %d args, got %d\n", name, fn->param_count, argc);
            return val_nil();
        }

        SymTable local = *table;
        for (i = 0; i < fn->param_count; i++) {
            Value v = eval_expr(args[i], table);
            sym_set(&local, fn->params[i], v);
        }

        ExecCtx ctx = {0};
        ctx.return_value = val_nil();
        eval_stmt_ctx(fn->body, &local, &ctx);

        if (ctx.has_return) return ctx.return_value;
        return val_nil();
    }

    fprintf(stderr, "Unknown function: %s\n", name);
    return val_nil();
}


static Value eval_list(AstNode *node, SymTable *table) {
    Value list = val_list();
    int i;

    for (i = 0; i < node->child_count; i++) {
        AstNode *child = node->children[i];
        if (child->kind == AST_KV) {
            if (child->child_count > 0) {
                Value v = eval_expr(child->children[0], table);
                if (list.data.list_val.count < MAX_ARRAY_ITEMS) {
                    Value *stored = (Value *)malloc(sizeof(Value));
                    *stored = v;
                    list.data.list_val.items[list.data.list_val.count++] = stored;
                }
            }
        } else {
            Value v = eval_expr(child, table);
            if (list.data.list_val.count < MAX_ARRAY_ITEMS) {
                Value *stored = (Value *)malloc(sizeof(Value));
                *stored = v;
                list.data.list_val.items[list.data.list_val.count++] = stored;
            }
        }
    }
    return list;
}

static Value eval_dict(AstNode *node, SymTable *table) {
    Value dict = val_dict();
    int i;

    for (i = 0; i < node->child_count; i++) {
        AstNode *child = node->children[i];
        if (child->kind == AST_KV && child->child_count > 0) {
            Value v = eval_expr(child->children[0], table);
            if (dict.data.dict_val.count < MAX_ARRAY_ITEMS) {
                dict.data.dict_val.keys[dict.data.dict_val.count] =
                    (char *)malloc(strlen(child->text) + 1);
                strcpy(dict.data.dict_val.keys[dict.data.dict_val.count], child->text);

                Value *stored = (Value *)malloc(sizeof(Value));
                *stored = v;
                dict.data.dict_val.values[dict.data.dict_val.count] = stored;
                dict.data.dict_val.count++;
            }
        }
    }
    return dict;
}

static Value eval_expr_ctx(AstNode *node, SymTable *table, ExecCtx *ctx) {
    if (!node) return val_nil();

    switch (node->kind) {

        case AST_BINARY: {
            Value l = eval_expr_ctx(node->children[0], table, ctx);
            Value r = eval_expr_ctx(node->children[1], table, ctx);
            Value out = val_nil();

            if (strcmp(node->text, "+") == 0) {
                if (l.kind == VAL_STRING || r.kind == VAL_STRING) {
                    const char *ls = val_to_string(&l);
                    char lbuf[MAX_TEXT]; strncpy(lbuf, ls, MAX_TEXT - 1); lbuf[MAX_TEXT - 1] = '\0';
                    const char *rs = val_to_string(&r);
                    char rbuf[MAX_TEXT]; strncpy(rbuf, rs, MAX_TEXT - 1); rbuf[MAX_TEXT - 1] = '\0';

                    char joined[MAX_TEXT];
                    snprintf(joined, sizeof(joined), "%s%s", lbuf, rbuf);
                    out = val_string(joined);
                } else if (l.kind == VAL_FLOAT || r.kind == VAL_FLOAT) {
                    out = val_float(val_to_num(&l) + val_to_num(&r));
                } else {
                    out = val_int((long)val_to_num(&l) + (long)val_to_num(&r));
                }
            } else if (strcmp(node->text, "-") == 0) {
                if (l.kind == VAL_FLOAT || r.kind == VAL_FLOAT) out = val_float(val_to_num(&l) - val_to_num(&r));
                else out = val_int((long)val_to_num(&l) - (long)val_to_num(&r));
            } else if (strcmp(node->text, "*") == 0) {
                if (l.kind == VAL_FLOAT || r.kind == VAL_FLOAT) out = val_float(val_to_num(&l) * val_to_num(&r));
                else out = val_int((long)val_to_num(&l) * (long)val_to_num(&r));
            } else if (strcmp(node->text, "/") == 0) {
                double denom = val_to_num(&r);
                if (denom == 0.0) {
                    fprintf(stderr, "Division by zero\n");
                    out = val_nil();
                } else if (l.kind == VAL_FLOAT || r.kind == VAL_FLOAT) {
                    out = val_float(val_to_num(&l) / denom);
                } else {
                    out = val_int((long)val_to_num(&l) / (long)denom);
                }
            } else if (strcmp(node->text, "==") == 0) {
                out = val_int(fabs(val_to_num(&l) - val_to_num(&r)) < 1e-12 ? 1 : 0);
            } else if (strcmp(node->text, "!=") == 0) {
                out = val_int(fabs(val_to_num(&l) - val_to_num(&r)) < 1e-12 ? 0 : 1);
            } else if (strcmp(node->text, "<") == 0) {
                out = val_int(val_to_num(&l) < val_to_num(&r) ? 1 : 0);
            } else if (strcmp(node->text, "<=") == 0) {
                out = val_int(val_to_num(&l) <= val_to_num(&r) ? 1 : 0);
            } else if (strcmp(node->text, ">") == 0) {
                out = val_int(val_to_num(&l) > val_to_num(&r) ? 1 : 0);
            } else if (strcmp(node->text, ">=") == 0) {
                out = val_int(val_to_num(&l) >= val_to_num(&r) ? 1 : 0);
            }

            val_free(&l);
            val_free(&r);
            return out;
        }

        case AST_STRING:
            return val_string(node->text);

        case AST_NUMBER: {
            if (strchr(node->text, '.')) {
                return val_float(strtod(node->text, NULL));
            } else {
                return val_int(strtol(node->text, NULL, 10));
            }
        }

        case AST_IDENT: {
            Value *v = sym_get(table, node->text);
            if (v) return *v;
            fprintf(stderr, "Undefined variable: %s\n", node->text);
            return val_nil();
        }

        case AST_CALL:
            return eval_call(node, table);

        case AST_LIST:
            return eval_list(node, table);

        case AST_DICT:
            return eval_dict(node, table);

        default:
            return val_nil();
    }
}

static int value_is_truthy(const Value *v) {
    if (v->kind == VAL_NIL) return 0;
    if (v->kind == VAL_INT) return v->data.int_val != 0;
    if (v->kind == VAL_FLOAT) return fabs(v->data.float_val) > 1e-12;
    if (v->kind == VAL_STRING) return v->data.str_val && v->data.str_val[0] != '\0';
    return 1;
}

static void eval_stmt_ctx(AstNode *node, SymTable *table, ExecCtx *ctx) {
    if (!node) return;
    if (ctx && ctx->has_return) return;

    switch (node->kind) {
        case AST_FUNC_DECL:
            register_function(node);
            break;

        case AST_RETURN:
            if (ctx) {
                if (node->child_count > 0) ctx->return_value = eval_expr_ctx(node->children[0], table, ctx);
                else ctx->return_value = val_nil();
                ctx->has_return = 1;
            }
            break;

        case AST_BLOCK: {
            int i;
            for (i = 0; i < node->child_count; i++) {
                eval_stmt_ctx(node->children[i], table, ctx);
                if (ctx && ctx->has_return) break;
            }
            break;
        }

        case AST_IF: {
            if (node->child_count >= 2) {
                Value cond = eval_expr_ctx(node->children[0], table, ctx);
                int ok = value_is_truthy(&cond);
                val_free(&cond);
                if (ok) eval_stmt_ctx(node->children[1], table, ctx);
            }
            break;
        }

        case AST_WHILE: {
            if (node->child_count >= 2) {
                while (!(ctx && ctx->has_return)) {
                    Value cond = eval_expr_ctx(node->children[0], table, ctx);
                    int ok = value_is_truthy(&cond);
                    val_free(&cond);
                    if (!ok) break;
                    eval_stmt_ctx(node->children[1], table, ctx);
                }
            }
            break;
        }

        case AST_ASSIGN: {
            if (node->child_count > 0) {
                Value v = eval_expr_ctx(node->children[0], table, ctx);
                sym_set(table, node->text, v);
            }
            break;
        }

        case AST_EXPR_STMT: {
            if (node->child_count > 0) {
                Value v = eval_expr_ctx(node->children[0], table, ctx);
                val_free(&v);
            }
            break;
        }

        case AST_PKG:
            /* keep your existing AST_PKG body here */
            break;

        default:
            break;
    }
}


static void register_function(AstNode *fn) {
    if (g_funcs.count >= 128) {
        fprintf(stderr, "Too many functions\n");
        exit(1);
    }

    FunctionDef *dst = &g_funcs.items[g_funcs.count++];
    memset(dst, 0, sizeof(*dst));
    strncpy(dst->name, fn->text, MAX_TEXT - 1);

    int i;
    int body_index = fn->child_count - 1;
    dst->body = fn->children[body_index];
    dst->param_count = body_index;

    if (dst->param_count > 16) dst->param_count = 16;
    for (i = 0; i < dst->param_count; i++) {
        strncpy(dst->params[i], fn->children[i]->text, MAX_TEXT - 1);
    }
}

static FunctionDef *find_function(const char *name) {
    int i;
    for (i = 0; i < g_funcs.count; i++) {
        if (strcmp(g_funcs.items[i].name, name) == 0) return &g_funcs.items[i];
    }
    return NULL;
}


static void eval_program(AstNode *node, SymTable *table) {
    int i;
    for (i = 0; i < node->child_count; i++) {
        eval_stmt(node->children[i], table);
    }
}

static void ast_free(AstNode *node) {
    if (!node) return;
    int i;
    for (i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node);
}

static char *read_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(size + 1);
    if (!buf) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        exit(1);
    }

    size_t read = fread(buf, 1, size, f);
    if (read != (size_t)size) {
        fprintf(stderr, "Error: failed to read file '%s'\n", filename);
        free(buf);
        fclose(f);
        exit(1);
    }

    buf[size] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char *argv[]) {
    const char *source;
    char *file_buf = NULL;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.tree>\n", argv[0]);
        return 1;
    }

    file_buf = read_file(argv[1]);
    source = file_buf;

    Lexer lx = {0};
    lx.src = source;
    lx.pos = 0;
    lx.line = 1;
    lx.col = 1;

    lex_all(&lx);

    Parser p = {0};
    p.tokens = lx.tokens;
    p.pos = 0;
    p.count = lx.count;

    AstNode *prog = parse_program(&p);

    SymTable table = {0};
        srand((unsigned int)time(NULL));
    eval_program(prog, &table);

    ast_free(prog);
    free(file_buf);
    return 0;
}
