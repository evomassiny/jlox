package com.craftinginterpreters.lox;

class Token {
    final TokenType type;
    // original matched string
    final String lexeme;
    // value for terminal
    final Object literal;
    // line in source where this token was scanned
    final int line;

    Token(TokenType type, String lexeme, Object literal, int line) {
        this.type = type;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line;
    }

    public String toString() {
        return this.type + " " + this.lexeme + " " + this.literal;
    }

}


