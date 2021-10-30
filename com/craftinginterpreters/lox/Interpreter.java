package com.craftinginterpreters.lox;

class Interpreter implements Expr.Visitor<Object> {


    /**
     * Execute Literal expressions.
     */
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
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
     * Execute Group expressions.
     */
    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return this.evaluate(expr.expression);
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
    public void interprete(Expr expr) {
        try {
            Object value = this.evaluate(expr);
            System.out.println(this.stringify(value));
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

