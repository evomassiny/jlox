package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final Stmt.Function declaration; // fnct ast
    private final Environment closure; // fnct ast

    LoxFunction(Stmt.Function declaration, Environment closure) {
        this.declaration = declaration;
        this.closure = closure;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment environment = new Environment(this.closure);
        for (int i = 0; i < this.declaration.params.size(); i++) {
            environment.define(
                this.declaration.params.get(i).lexeme,
                arguments.get(i)
            );
        }
        try {
            interpreter.executeBlock(this.declaration.body, environment);
        } catch (Return returnValue) {
            return returnValue.value;
        }
        return null;
    }

    @Override
    public int arity() {
        return this.declaration.params.size();
    }

    @Override
    public String toString() {
        return "<fn " + this.declaration.name.lexeme + " >";
    }

}
