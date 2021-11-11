package com.craftinginterpreters.lox;

class Return extends RuntimeException {
    final Object value;

    Return(Object value) {
        super(null, null, false, false); // disable stack-trace and stuff.
        this.value = value;
    }
}
