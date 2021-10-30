package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.List;

import static com.craftinginterpreters.lox.TokenType.*;

class Parser {
    private static class ParseError extends RuntimeException {}

    private final List<Token> tokens;
    private int current = 0;

    public Parser(List<Token> tokens) {
        this.tokens = tokens;
    }


    /**
     * Main function: parse a list of tokens into 
     * a collection of Statements.
     */
    public List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<Stmt>();
        while (!this.isAtEnd()) {
            statements.add(this.declaration());
        }
        return statements;
    }

    // statement -> exprStmt | printStmt | block;
    private Stmt statement() {
        if (this.match(PRINT)) {
            return this.printStatement();
        }
        if (this.match(LEFT_BRACE)) {
            return new Stmt.Block(this.block());
        }
        return this.expressionStatement();
    }

    // printStmt -> "print" expression ";" ;
    private Stmt printStatement() {
        Expr value = this.expression();
        this.consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);

    }

    // varDecl -> 'var' IDENTIFIER ( "=" expression )? ';' ; 
    private Stmt varDeclaration() {
        Token name = this.consume(IDENTIFIER, "Expect variable name.");
        
        Expr initializer = null;
        if (this.match(EQUAL)) {
            initializer = this.expression();
        }
        this.consume(SEMICOLON, "Expect ';' after var declaration.");
        return new Stmt.Var(name, initializer);
    }

    // exprStmt -> expression ';' ;
    private Stmt expressionStatement() {
        Expr expr = this.expression();
        this.consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

   // block -> "{" declaration * "}"' ;
    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<Stmt>();
        while (!this.check(RIGHT_BRACE) && !this.isAtEnd()) {
            statements.add(this.declaration());
        }
        this.consume(RIGHT_BRACE, "Expect '}' after block.");
        return statements;
    }

    // expression -> equality
    private Expr expression() {
        return this.assignment();
    }

    // assignment -> IDENTIFIER "=" assignment | equality ;
    private Expr assignment() {
        // parse expr, at this point we don't know if it's an
        // l-value (binding label) or an r-value (value)
        Expr expr = this.equality();

        if (this.match(IDENTIFIER)) {
            Token equals = this.previous();
            Expr r_value = this.assignment();
            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable)expr).name;
                return new Expr.Assign(name, r_value);
            }
            this.error(equals, "Invalid assignement target.");
        }
        return expr;
    }

    private Stmt declaration() {
        try {
            if (this.match(VAR)) 
                return this.varDeclaration();
            return this.statement();
        } catch (ParseError error) {
            this.synchronize(); // move to next valid expr/stmt
            return null;
        }
    }

    // equality -> comparison ( ( "!=" | "==" ) comparison )* ;
    private Expr equality() {
        Expr expr = this.comparison();

        while (this.match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = this.previous();
            Expr right = this.comparison();
            //left associative
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }


    // comparison -> term ( ( ">" | ">=" | "<" | "<="  ) term )* ;
    private Expr comparison() {
        Expr expr = this.term();

        while (this.match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = this.previous();
            Expr right = this.term();
            //left associative
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // term -> factor ( ( "+" | "-" ) factor)*;
    private Expr term() {
        Expr expr = this.factor();
        while (this.match(PLUS, MINUS)) {
            Token operator = this.previous();
            Expr right = this.factor();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // factor -> unary ( ( "*" | "/" ) unary)*;
    private Expr factor() {
        Expr expr = this.unary();
        while (this.match(STAR, SLASH)) {
            Token operator = this.previous();
            Expr right = this.unary();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // unary -> ( "!" | "-" ) unary | primary;
    private Expr unary() {
        if (this.match(BANG, MINUS)) {
            Token operator = this.previous();
            Expr right = this.unary();
            return new Expr.Unary(operator, right);
        }
        return this.primary();
    }

    // primary -> NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER;
    private Expr primary() {
        if (this.match(FALSE)) return new Expr.Literal(false);
        if (this.match(TRUE)) return new Expr.Literal(true);
        if (this.match(NIL)) return new Expr.Literal(null);

        if (this.match(NUMBER, STRING)) {
            return new Expr.Literal(this.previous().literal);
        }

        if (this.match(IDENTIFIER)) return new Expr.Variable(this.previous());

        if (this.match(LEFT_PAREN)) {
            // parse sub expression
            Expr expr = this.expression();
            // assert that the following 
            // token is a ")"
            this.consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }
        throw this.error(this.peek(), "Expect expression.");
    }

    // return true if any type match current.
    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (this.check(type)) {
                this.advance();
                return true;
            }
        }
        return false;
    }

    private boolean check(TokenType type) {
        if (this.isAtEnd()) {
            return false;
        }
        return this.peek().type == type;
    }

    private boolean isAtEnd() {
        return this.peek().type == EOF;
    }

    private Token advance() {
        if (!this.isAtEnd()) {
            // assert that we never go beyond EOF
            this.current ++;
        }
        return this.previous();
    }

    private Token peek() {
        return this.tokens.get(this.current);
    }

    private Token previous() {
        return this.tokens.get(this.current -1);
    }

    // consome next token, if it doesn't match `type`,
    // throws a ParseError.
    private Token consume(TokenType type, String message) {
        if (this.check(type)) return this.advance();
        throw this.error(this.peek(), message);
    }

    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    // called when a ParseError is detected, ignore 
    // every following tokens until we reach the begining
    // of a new statement.
    private void synchronize() {
        this.advance();
        while (!this.isAtEnd()) {
            if (this.previous().type == SEMICOLON) return;

            switch (peek().type) {
                case CLASS:
                case FOR:
                case FUN:
                case IF:
                case PRINT:
                case RETURN:
                case VAR:
                case WHILE:
                    return;
            }
            this.advance();
        }

    }

}
