package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.craftinginterpreters.lox.TokenType.*;

class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();
    private int start = 0;
    private int current = 0;
    private int line = 1; // natural

    private static final Map<String, TokenType> keywords;
    static {
        keywords = new HashMap<>();
        keywords.put("and", AND);
        keywords.put("class", CLASS);
        keywords.put("else", ELSE);
        keywords.put("false", FALSE);
        keywords.put("fun", FUN);
        keywords.put("for", FOR);
        keywords.put("if", IF);
        keywords.put("nil", NIL);
        keywords.put("or", OR);
        keywords.put("print", PRINT);
        keywords.put("return", RETURN);
        keywords.put("super", SUPER);
        keywords.put("this", THIS);
        keywords.put("true", TRUE);
        keywords.put("var", VAR);
        keywords.put("while", WHILE);

    }

    Scanner(String source) {
        this.source = source;
    }

    List<Token> scanTokens() {
        while (!isAtEnd()) {
            // we are at the beginning of the next lexeme
            this.start = this.current;
            this.scanToken();
        }
        this.tokens.add(new Token(EOF, "", null, line));
        return tokens;
    }

    private void scanToken() {
        char c = this.advance();
        switch (c) {

            // single chars
            case '(':
                this.addToken(LEFT_PAREN);
                break;
            case ')':
                this.addToken(RIGHT_PAREN);
                break;
            case '[':
                this.addToken(LEFT_BRACE);
                break;
            case ']':
                this.addToken(RIGHT_BRACE);
                break;
            case ',':
                this.addToken(COMMA);
                break;
            case '.':
                this.addToken(DOT);
                break;
            case '-':
                this.addToken(MINUS);
                break;
            case '+':
                this.addToken(PLUS);
                break;
            case ';':
                this.addToken(SEMICOLON);
                break;
            case '*':
                this.addToken(STAR);
                break;
                // div or comment
            case '/':
                if (match('/')) {
                    while (this.peek() != '\n' && !this.isAtEnd()) this.advance();
                } else {
                    this.addToken(SLASH);
                }
                break;

                // one or 2 chars
            case '!':
                this.addToken(match('=') ? BANG_EQUAL: BANG);
                break;
            case '=':
                this.addToken(match('=') ? EQUAL_EQUAL: EQUAL);
                break;
            case '>':
                this.addToken(match('=') ? GREATER_EQUAL: GREATER);
                break;
            case '<':
                this.addToken(match('=') ? LESS_EQUAL: LESS);
                break;
                // Litterals
            case '"':
                this.string();
                break;

                // whitespaces
            case ' ':
            case '\r':
            case '\t':
                // ignore whitespaces
                break;
            case '\n':
                this.line++;
                break;
            default:
                if (this.isDigit(c)) {
                    this.number();
                } else if (this.isAlpha(c)) {
                    this.identifier();
                } else {
                    Lox.error(line, "Unexpected character.");
                }
                break;
        }

    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private boolean isAlpha(char c) {
        return  (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return this.isAlpha(c) || this.isDigit(c);
    }

    private void identifier() {
        while (this.isAlphaNumeric(this.peek())) this.advance();
        // check if the identifier is not a reserved keyword
        String text = this.source.substring(start, current);
        TokenType type = this.keywords.get(text);
        if (type == null) type = IDENTIFIER;
        this.addToken(type);
    }

    private void number() {
        while (this.isDigit(this.peek())) this.advance();
        // factional part
        if (this.peek() == '.' && this.isDigit(this.peekNext())) {
            // consume "."
            this.advance();
        }
        while (this.isDigit(this.peek())) this.advance();

        this.addToken(NUMBER, Double.parseDouble(this.source.substring(this.start, this.current)));
    }

    private void string() {
        while (this.peek() != '"') {
            if (this.isAtEnd()) {
                Lox.error(this.line, "Unterminated string.");
            }
            if (this.peek() == '\n') this.line++;
            this.advance();
        }

        this.advance(); // consume the closing '"'

        // Trim surrounding quotes
        String value = this.source.substring(this.start + 1, this.current - 1);
        this.addToken(STRING, value);
    }

    /** peek next char, return true if it matches `expected`.
     ** consume it if so.
     **/
    private boolean match(char expected) {
        if (this.isAtEnd()) return false;
        if (this.source.charAt(this.current) != expected) return false;
        this.current++;
        return true;
    }

    private char peek() {
        if (this.isAtEnd()) return '\0';
        return this.source.charAt(this.current);
    }

    private char peekNext() {
        if (this.current + 1 >= this.source.length()) return '\0';
        return this.source.charAt(this.current + 1);
    }

    private boolean isAtEnd() {
        return this.current >= this.source.length();
    }

    private char advance() {
        return this.source.charAt(this.current++);
    }

    private void addToken(TokenType type) {
        this.addToken(type, null);
    }

    private void addToken(TokenType type, Object literal) {
        String text = this.source.substring(start, current);
        this.tokens.add(new Token(type, text, literal, line));
    }
}
