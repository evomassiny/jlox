package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.List;

class Interpreter implements Expr.Visitor<Object>,
                             Stmt.Visitor<Void> {
    public Environment globals = new Environment();
    private Environment environment = globals;

    Interpreter() {
        // add native functions
        globals.define(
            "clock",
            new LoxCallable() {

                @Override
                public int arity() { return 0; }

                @Override
                public Object call(Interpreter intereter, List<Object> args) {
                    return (double) System.currentTimeMillis() / 1000.0;
                }

                @Override
                public String toString() {
                    return "<native fn>";
                }
            }
        );
    }

    /**
     * Execute Literal expressions.
     */
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    /**
     * Execute Logical expressions (OR / AND).
     */
    @Override
    public Object visitLogicalExpr(Expr.Logical expr) {
        Object left = this.evaluate(expr.left);

        if (expr.operator.type == TokenType.OR) {
            if (this.isThruthy(left)) return left;
        } else if (expr.operator.type == TokenType.AND ) {
            if (!this.isThruthy(left)) return left;
        } else {
            throw new RuntimeError(
                    expr.operator,
                    "Operator does not represents a Logical expression."
                    );
        }
        // python style, return any value kind.
        return this.evaluate(expr.right);
    }

    /**
     * Execute a "Variable" expression, basically load its value.
     */
    @Override
    public Object visitVariableExpr(Expr.Variable expr) {
        return this.environment.get(expr.name);
    }

    /**
     * Execute Binary expressions.
     */
    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object right = this.evaluate(expr.right);
        Object left = this.evaluate(expr.left);
        switch (expr.operator.type) {
            case MINUS:
                this.checkNumberOperands(expr.operator, right, left);
                return (double)left - (double)right;
            case PLUS:
                if (left instanceof Double && right instanceof Double) {
                    return (double)left + (double)right;
                }
                if (left instanceof String && right instanceof String) {
                    return (String)left + (String)right;
                }
                throw new RuntimeError(expr.operator, "Operands must be 2 numbers or 2 strings");
            case SLASH:
                this.checkNumberOperands(expr.operator, right, left);
                return (double)left / (double)right;
            case STAR:
                this.checkNumberOperands(expr.operator, right, left);
                return (double)left * (double)right;
            case GREATER:
                this.checkNumberOperands(expr.operator, right, left);
                return (double)left > (double)right;
            case GREATER_EQUAL:
                this.checkNumberOperands(expr.operator, right, left);
                return (double)left >= (double)right;
            case LESS:
                this.checkNumberOperands(expr.operator, right, left);
                return (double)left < (double)right;
            case LESS_EQUAL:
                this.checkNumberOperands(expr.operator, right, left);
                return (double)left <= (double)right;
            case BANG_EQUAL:
                return !this.isEqual(left, right);
            case EQUAL_EQUAL:
                return this.isEqual(left, right);
        }
        return null;
    }

    /**
     * Execute function calls
     */
    @Override
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = this.evaluate(expr.callee);

        List<Object> arguments = new ArrayList<Object>();
        for (Expr argument: expr.arguments) {
            arguments.add(this.evaluate(argument));
        }
        if (!(callee instanceof LoxCallable)) {
            throw new RuntimeError(expr.paren, "Can only call functions and classes.");
        }
        LoxCallable function = (LoxCallable)callee;
        if (arguments.size() != function.arity()) {
            throw new RuntimeError(expr.paren,
                    "Expected " + function.arity() + " arguments "
                    + "but got " + arguments.size() + "."
                );
        }
        return function.call(this, arguments);
    }

    /**
     * Execute Group expressions.
     */
    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return this.evaluate(expr.expression);
    }

    /**
     * Execute Assign expressions.
     */
    @Override
    public Object visitAssignExpr(Expr.Assign expr) {
        Object value = this.evaluate(expr.value);
        this.environment.assign(expr.name, value);
        return value;
    }

    /**
     * Execute Unary expressions.
     */
    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = this.evaluate(expr.right);
        switch (expr.operator.type) {
            case MINUS:
                this.checkNumberOperand(expr.operator, right);
                return -(double)right;
            case BANG:
                return !this.isThruthy(right);
        }
        return null; // unreachable
    }

    /**
     * Evaluate expression
     */
    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    /**
     * Execute variable declaration
     */
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        Object value = null;
        if (stmt.initializer != null) {
            value = this.evaluate(stmt.initializer);
        }
        this.environment.define(stmt.name.lexeme, value);
        return null;
    }

    /**
     * Evaluate a list of statements in the provided environment.
     */
    public void executeBlock(List<Stmt> statements, Environment environment) {
        Environment previous = this.environment;
        try {
            this.environment = environment;
            for(Stmt statement: statements) {
                this.execute(statement);
            }
        } catch (RuntimeError error) {

        } finally {
            this.environment = previous;
        }
    }

    // evaluate block
    @Override
    public Void visitBlockStmt(Stmt.Block block) {
        // create new env, linked to the current one.
        this.executeBlock(block.statements, new Environment(this.environment));
        return null;
    }
    
    // evaluate if condtion, and the corresponding block
    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        // evaluate condition and cast as bool
        if (this.isThruthy(this.evaluate(stmt.condition))) {
            this.execute(stmt.thenBranch);
        } else {
            if (stmt.elseBranch != null) {
                this.execute(stmt.elseBranch);
            }
        }
        return null;
    }

    // evaluate Statement
    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        this.evaluate(stmt.expression);
        return null;
    }

    // evaluate function declaration
    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        LoxFunction function = new LoxFunction(stmt, this.environment);
        environment.define(stmt.name.lexeme, function);
        return null;
    }

    // evaluate Print Statement
    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        Object value = this.evaluate(stmt.expression);
        System.out.println(this.stringify(value));
        return null;
    }

    // evaluate Return Statement
    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = null;
        if (stmt.value != null) {
            value = this.evaluate(stmt.value);
        }
        throw new Return(value);
    }

    // evaluate While Statement
    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        while (this.isThruthy(this.evaluate(stmt.condition))) {
            this.execute(stmt.body); 
        }
        return null;
    }

    /**
     * Evaluate a statement, no matter its kind.
     */
    private void execute(Stmt stmt) {
        stmt.accept(this);
    }

    /**
     * Evaluate an Object into a Boolean
     */
    private boolean isThruthy(Object object) {
        if (object == null) return false;
        if (object instanceof Boolean) return (boolean)object;
        return true;
    }

    private boolean isEqual(Object left, Object right) {
        if (left == null && right == null) return true;
        if (left == null) return false;
        return left.equals(right);
    }

    /**
     * Assert that the object `operand` type can be evaluated as a number.
     */
    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) return;
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    /**
     * Assert that boths objects `left` and `right` type can be evaluated as numbers.
     */
    private void checkNumberOperands(Token operator, Object left, Object right) {
        if (left instanceof Double && right instanceof Double) return;
        throw new RuntimeError(operator, "Operands must be a number.");
    }

    /**
     * Evalute `expr` and print the result.
     */
    public void interpret(List<Stmt> statements) {
        try {
            for (Stmt statement: statements) {
                this.execute(statement);
            }
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    private String stringify(Object object) {
        if (object == null) return "nil";
        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }
        // boolean + strings
        return object.toString();
    }

}

