package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;


class Environment {
    //  parent scope, in scope chain.
    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    Environment() {
        this.enclosing = null;
    }

    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    public void define(String name, Object value) {
        this.values.put(name, value);
    }

    public Object getAt(int distance, String name) {
        return this.ancestor(distance).values.get(name);
    }

    public void assignAt(int distance, Token name, Object value) {
        this.ancestor(distance).assign(name, value);
    }

    public Environment ancestor(int distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }
        return environment;
    }

    public void assign(Token name, Object value) {
        if (this.values.containsKey(name.lexeme)) {
            this.values.put(name.lexeme, value);
            return;
        }
        // climb the scope chain, `name` could be declared in an outer scope.
        if (this.enclosing != null) {
            this.enclosing.assign(name, value);
            return;
        }
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme +"'.");
    }

    public Object get(Token name) {
        // Why token instead of String ? to locate the identifier in error msg
        if (this.values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }
        // climb the scope chain, `name` could be declared in an outer scope.
        if (this.enclosing != null) {
            return this.enclosing.get(name);
        }
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme +"'.");
    }
}
