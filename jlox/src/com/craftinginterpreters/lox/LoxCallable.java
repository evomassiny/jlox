package com.craftinginterpreters.lox;

import java.util.List;

interface LoxCallable {
    // returns the number of argument expected by the Callable
    int arity();
    Object call(Interpreter interpreter, List<Object> arguments);
}
