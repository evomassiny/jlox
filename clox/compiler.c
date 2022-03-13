#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "object.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

// precedence order
// NOTE: higher precedence means less expressions.
// eg: in `A*B+C`, * concerns 2 expressions, + concerns 4
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // ==, !=
  PREC_COMPARISON, // <, >, <=, <=
  PREC_TERM,       // +, -
  PREC_FACTOR,     // *, /
  PREC_UNARY,      // !, -
} Precedence;

typedef void (*ParseFn)(
    bool canAssign); // ParseFn: `fn(void) -> void` function address.

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

// global parser state.
Parser parser;
Compiler *current = NULL;
Chunk *compilingChunk;

static Chunk *currentChunk() { return compilingChunk; }

static void errorAt(Token *token, const char *message) {
  // only print one error.
  if (parser.panicMode)
    return;
  parser.panicMode = true;
  // print location:
  fprintf(stderr, "[line %d] Error", token->line);
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end.");
  } else if (token->type == TOKEN_ERROR) {
    // nothing
  } else {
    // because `token->start` might not be NULL termnated,
    // we need to specify the length to display,
    // we do  this using (`%.3s`, str),
    // or we can specify it dynamically using ('%.*s', 3, str)
    fprintf(stderr, " at '%.*s' (%s)\n", token->length, token->start,
            tokenTypeToStr(token->type));
  }

  // print message:
  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char *message) { errorAt(&parser.previous, message); }

static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;
    errorAtCurrent(parser.current.start);
  }
}

/**
 * Consume one token, panic if it is not of kind `type`.
 */
static void consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

static bool check(TokenType type) { return type == parser.current.type; }

/*
 * advance scanner state only if matches.
 */
static bool match(TokenType type) {
  if (!check(type))
    return false;
  advance();
  return true;
}

/** write 1 bytes to current chunk.
 */
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

/**
 * Emit `instruction` followed by two place holders
 * which must be later replaced by a jump offset (16bits).
 *
 * Return the index of the jump instruction.
 */
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff); // placeholders
  emitByte(0xff);
  return currentChunk()->count - 2;
}

/**
 * write 2 bytes to current chunk.
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);
  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) {
    error("Loop body too large.");
  }
  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

/**
 * write RETURN instruction to current chunk
 */
static void emitReturn() {
  emitByte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunks.");
    return 0;
  }
  return (uint8_t)constant;
}

/**
 * Store the constant in the chunk table and,
 * emit 2 bytes:
 * * the OP_CONSTANT,
 * * the index of the constant in the table.
 */
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

/** replace the placeholder placed after `offset`,
 * with the number of byte code needed to jump
 * to the current last bytecode index.
 */
static void patchJump(int offset) {
  // 2: size of the bytes holding the jump offset itself
  int jump = currentChunk()->count - offset - 2;
  if (jump > UINT16_MAX) {
    error("Cannot jump this far !");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler *compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  current = compiler;
}

static void endCompiler() { emitReturn(); }

static void beginScope() { current->scopeDepth++; }
static void endScope() {
  current->scopeDepth--;
  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
    emitByte(OP_POP); // drop locals at the end of the scope.
    current->localCount--;
  }
}

// forward declarations
static void and_(bool _canAssign);
static void expression();
static void statement();
static void declaration();
static uint8_t identifierConstant(Token *name);
static int resolveLocal(Compiler *compiler, Token *name);
static ParseRule *getRule(TokenType type);
static void parsePrecendence(Precedence precedence);

/**
 * assumes that the left operand was already consumed (and compiled),
 * and the infix operator was also consumed.
 */
static void binary(bool _canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  // parse as higher precedence
  parsePrecendence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emitByte(OP_SUBSTRACT);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIVIDE);
    break;
  default:
    return; // Unreachable
  }
}

/**
 * infix expression.
 * Push the literal directly onto the chunk for simple
 * values,
 * Not sure yet for str...
 */
static void literal(bool _canAssign) {
  TokenType operatorType = parser.previous.type;
  switch (operatorType) {
  case TOKEN_NIL:
    emitByte(OP_NIL);
    break;
  case TOKEN_TRUE:
    emitByte(OP_TRUE);
    break;
  case TOKEN_FALSE:
    emitByte(OP_FALSE);
    break;
  default:
    return; // Unreachable
  }
}

/**
 * Assumes leading '(' is already consumed.
 */
static void grouping(bool _canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool _canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

/**
 * emit bytes that skip right hand of 'or' expression if the
 * evaluated left hand value is truthy, without droping the value.
 *
 * Assumes the left hand was already compiled.
 */
static void or_(bool _canAssign) {
  // this emulates an OP_JUMP_IF_TRUE
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);
  parsePrecendence(PREC_OR);
  patchJump(endJump);
}

static void string(bool _canAssign) {
  // we copy fron the second char, to trim the leading '"',
  // for the same reason, we shorten the string lenght by 2
  emitConstant(OBJ_VAL(
      copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

/**
 * Resolve a variable "store" or "load",
 * * globals are resolved at runtime using their name (stored in the constant
 * array)
 * * locals are resolved at compile time using their value index in the stack
 */
static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  // get (runtime) stack slot index, or -1 if global
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else {
    // get var name in constant table
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void unary(bool _canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand
  parsePrecendence(PREC_UNARY);

  // Emit operator instruction
  switch (operatorType) {
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  default:
    return; // Unreachable (?)
  }
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS] = {unary, binary, PREC_TERM},
        [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG] = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
        [TOKEN_STRING] = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
        [TOKEN_AND] = {NULL, and_, PREC_AND},
        [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
        [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
        [TOKEN_IF] = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL] = {literal, NULL, PREC_NONE},
        [TOKEN_OR] = {NULL, or_, PREC_OR},
        [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
        [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

/**
 * Parse all expression until we reach a token associated
 * with an higher precedence than `precedence`.
 *
 * Assumes that the first token is the starting point of a
 * prefix expression.
 */
static void parsePrecendence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  // call the rule associated with handling
  // expressing **STARTING** with this token
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    // call the rule associated with handling
    // expressing **CONTAINING** this token
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignement target.");
  }
}

static uint8_t identifierConstant(Token *name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b) {
  return (a->length == b->length) && memcmp(a->start, b->start, a->length) == 0;
}

/**
 * Return the runtime stack slot index containing the
 * the local value.
 * This relies on the facts:
 * * only "var declarations" leave a value onto the stack,
 * * the "declaration" order follow the one from the runtime.
 *
 * # Return
 * * return -1 if no local variable was declared using this name
 * in this scope or its parents' ones.
 */
static int resolveLocal(Compiler *compiler, Token *name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (identifiersEqual(&local->name, name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

/**
 * stores the name into the table of locals, along with
 * its scope depth.
 * (table only used at compile time)
 */
static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function (max 256).");
    return;
  }
  Local *local = &current->locals[current->localCount++];
  local->name = name;
  // represents that local is in "declaration" state.
  // the sole purpose of this sentinel value is to handle this edge case;
  // ```
  // var a = 1;
  // {
  //   var a = a;
  // }
  // ```
  local->depth = -1;
}

/**
 * Declaring a variable means adding it to the scope,
 * (once added, it is not usable until we "define" it).
 *
 * Mutate the compiler state, does not emit any code.
 */
static void declareVariable() {
  if (current->scopeDepth == 0)
    return;
  Token *name = &parser.previous;
  // iter all already declared variable in the current scope
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    // detect change of scope
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }
    // If 2 variables are declared with the same name in the same scope
    // we trigger an error.
    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }
  addLocal(*name);
}

static uint8_t parseVariable(const char *errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  declareVariable();

  // locals can be resolved statically,
  // we store their value directly into the VM stack
  // and replace their reference by their index in the above mentioned
  // stack.
  if (current->scopeDepth > 0)
    return 0;

  // because globals can be used before (lexically) being
  // declared, and because this is a single pass compiler
  // we need to look them up dynamically (at run time),
  // using an HashMap, we store the "key" (variable name) in
  // the constant array.
  //
  // At runtime, an opcode tells the VM to load a
  // constant value from the constant array, then read/write
  // the value from/into the `vm.globals` hashmap
  return identifierConstant(&parser.previous);
}

static void markInitialized() {
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/**
 * Emit instructions to create a variable binding.
 */
static void defineVariable(uint8_t global) {
  // if we are defining a local variable,
  // the value it binds to is already in the stack.
  // The value stack index is enough to define a local variable,
  // so there is byte to emit.
  if (current->scopeDepth > 0) {
    markInitialized(); // But, we can mark it as "usable"
    return;
  }
  // Global variables are defined by name,
  // so we store the name in the constant table
  emitBytes(OP_DEFINE_GLOBAL, global);
}

/**
 * Assume left hand of 'and' expression was already compiled,
 * emit byte that jump over right hand expression
 * if the left hand value is falsey. (but keep the value on stack).
 */
static void and_(bool _canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecendence(PREC_AND);
  patchJump(endJump);
}

/**
 * Lookup ParseRule in global table.
 * (workaround definition cycle).
 */
static ParseRule *getRule(TokenType type) { return &rules[type]; }

static void expression() { parsePrecendence(PREC_ASSIGNMENT); }

/**
 * parse block block statement (assume first '{' was consumed).
 */
static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/** assume a 'VAR' token has already been consumed.
 */
static void varDeclaration() {

  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  defineVariable(global);
}

/**
 * compile expression, and pop its value.
 * Assumes that all expressions push a value.
 */
static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

/**
 * Assume `for token has already been consumed.
 */
static void forStatement() {
  beginScope(); // to reduced the scope of any var declaration in the
                // initializer
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  // INITIALIZER
  if (match(TOKEN_SEMICOLON)) {
    ; // no initializer
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  // CONDITION CLAUSE
  // label before condition
  int loopStart = currentChunk()->count;
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // drop condition value
  }

  // INCREMENT CLAUSE
  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP); // we only run the expression for side effects
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clause.");
    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  // for body
  statement();

  emitLoop(loopStart);
  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // drop condition value
  }

  endScope();
}

/** Compile IF statement,
 * assumes "if" was consumed.
 */
static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condiftion.");

  // 'then' branch
  // emit a goto 'placeholder'
  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP); // drop condition evaluation from vm stack
  statement();

  int elseJump = emitJump(OP_JUMP);

  // replace 'placeholder' with actual address
  patchJump(thenJump);

  // 'else' branch
  emitByte(OP_POP); // drop condition evaluation from vm stack
  if (match(TOKEN_ELSE))
    statement();

  patchJump(elseJump);
}

/**
 * Assume "print" token was already consumed.
 */
static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

/**
 * Assumes "while" token was already consumed.
 */
static void whileStatement() {
  int loopStart = currentChunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'while' condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

// Skip tokens until we encounter the start of a new statement.
static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) {
      return;
    }
    switch (parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;
    default:; // no op
    }
    advance();
  }
}

/**
 * declaration -> varDeclaration
 *                | statement;
 */
static void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode)
    synchronize();
}

/**
 * statement -> printStatement
 *              | expressionStatement
 *              | ifStatement
 *              | whileStatement
 *              | forStatement
 *              | block;
 */
static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope(); // mutates "current" (Compiler)
    block();
    endScope();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else {
    expressionStatement();
  }
}

/**
 * Compile source into bytecode, in a single pass.
 * mutates globals "vm", "scanner", ""current", "compilingChunk"
 *
 * Return false on error.
 */
bool compile(const char *source, Chunk *chunk) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  endCompiler();
  return !parser.hadError;
}
