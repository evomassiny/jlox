package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final Stmt.Function declaration; // fnct ast
    private final Environment closure; // fnct ast
    private final boolean isInitializer;

    LoxFunction(Stmt.Function declaration, Environment closure, boolean isInitializer) {
        this.declaration = declaration;
        this.closure = closure;
        this.isInitializer = isInitializer;
    }

    // Build a clone of self, but append an environment to its closure
    // chain, it this env, define "this" as `instance`
    LoxFunction bind(LoxInstance instance) {
        Environment environment = new Environment(closure);
        environment.define("this", instance);
        return new LoxFunction(this.declaration, environment, this.isInitializer);
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
            if (this.isInitializer) return this.closure.getAt(0, "this");
            return returnValue.value;
        }
        // constructor return the  build instance
        if (this.isInitializer) {
            return this.closure.getAt(0, "this");
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
