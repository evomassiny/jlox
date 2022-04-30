#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
  const char *start;
  const char *current;
  int line;

} Scanner;

Scanner scanner;

void initScanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static bool isAtEnd(void) { return *scanner.current == '\0'; }

static char advance() {
  return *scanner.current++;
  /*return scanner.current[-1];*/
}

static char peek() { return *scanner.current; }

static char peekNext() {
  if (isAtEnd())
    return '\0';
  return scanner.current[1];
}

static bool match(char expected) {
  if (isAtEnd())
    return false;
  if (*scanner.current != expected)
    return false;
  scanner.current++;
  return true;
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token errorToken(const char *error) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = error; // doesn't this points to stack allocated memory ??!
  token.length = (int)strlen(error);
  token.line = scanner.line;
  return token;
}

static void skipWhitespace(void) {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      scanner.line++;
      advance();
      break;
    case '/':
      if (peekNext() == '/') {
        while (peek() != '\n' && !isAtEnd())
          advance();
      } else {
        return;
      }
    default:
      return;
    }
  }
}

static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  // hand-crafted DFA
  switch (scanner.start[0]) {
  case 'a':
    return checkKeyword(1, 2, "nd", TOKEN_AND);
  case 'c':
    return checkKeyword(1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return checkKeyword(1, 3, "lse", TOKEN_ELSE);
  case 'f':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return checkKeyword(2, 1, "r", TOKEN_FOR);
      case 'u':
        return checkKeyword(2, 1, "n", TOKEN_FUN);
      }
    }
    break;
  case 'i':
    return checkKeyword(1, 1, "f", TOKEN_IF);
  case 'n':
    return checkKeyword(1, 2, "il", TOKEN_NIL);
  case 'o':
    return checkKeyword(1, 1, "r", TOKEN_OR);
  case 'p':
    return checkKeyword(1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return checkKeyword(1, 4, "uper", TOKEN_SUPER);
  case 't':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'h':
        return checkKeyword(2, 2, "is", TOKEN_FOR);
      case 'r':
        return checkKeyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  case 'v':
    return checkKeyword(1, 2, "ar", TOKEN_VAR);
  case 'w':
    return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek()))
    advance();
  return makeToken(identifierType());
}

static Token number(void) {
  while (isDigit(peek()))
    advance();

  // look for fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // consume '.'
    advance();
    while (isDigit(peek()))
      advance();
  }
  return makeToken(TOKEN_NUMBER);
}

static Token string(void) {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n')
      scanner.line++;
    advance();
  }

  if (isAtEnd())
    return errorToken("Unterminated string.");

  // the closing quote
  advance();
  return makeToken(TOKEN_STRING);
}

Token scanToken(void) {
  skipWhitespace();

  scanner.start = scanner.current;
  if (isAtEnd())
    return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c))
    return identifier();
  if (isDigit(c))
    return number();
  switch (c) {
  case '(':
    return makeToken(TOKEN_LEFT_PAREN);
  case ')':
    return makeToken(TOKEN_RIGHT_PAREN);
  case '}':
    return makeToken(TOKEN_RIGHT_BRACE);
  case '{':
    return makeToken(TOKEN_LEFT_BRACE);
  case ';':
    return makeToken(TOKEN_SEMICOLON);
  case ',':
    return makeToken(TOKEN_COMMA);
  case '.':
    return makeToken(TOKEN_DOT);
  case '-':
    return makeToken(TOKEN_MINUS);
  case '+':
    return makeToken(TOKEN_PLUS);
  case '/':
    return makeToken(TOKEN_SLASH);
  case '*':
    return makeToken(TOKEN_STAR);
  case '!':
    return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return makeToken(match('<') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return makeToken(match('<') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return string();
  }

  return errorToken("Unexpected echaracter.");
}

/** returns a static string representing the `type`.
 */
const char *tokenTypeToStr(TokenType type) {
#define AS_STR(type)                                                           \
  case type:                                                                   \
    return #type;
  switch (type) {
    AS_STR(TOKEN_LEFT_PAREN)
    AS_STR(TOKEN_RIGHT_PAREN)
    AS_STR(TOKEN_LEFT_BRACE)
    AS_STR(TOKEN_RIGHT_BRACE)
    AS_STR(TOKEN_COMMA)
    AS_STR(TOKEN_DOT)
    AS_STR(TOKEN_MINUS)
    AS_STR(TOKEN_PLUS)
    AS_STR(TOKEN_SEMICOLON)
    AS_STR(TOKEN_SLASH)
    AS_STR(TOKEN_STAR)
    AS_STR(TOKEN_BANG)
    AS_STR(TOKEN_BANG_EQUAL)
    AS_STR(TOKEN_EQUAL)
    AS_STR(TOKEN_EQUAL_EQUAL)
    AS_STR(TOKEN_GREATER)
    AS_STR(TOKEN_GREATER_EQUAL)
    AS_STR(TOKEN_LESS)
    AS_STR(TOKEN_LESS_EQUAL)
    AS_STR(TOKEN_IDENTIFIER)
    AS_STR(TOKEN_STRING)
    AS_STR(TOKEN_NUMBER)
    AS_STR(TOKEN_AND)
    AS_STR(TOKEN_CLASS)
    AS_STR(TOKEN_ELSE)
    AS_STR(TOKEN_FALSE)
    AS_STR(TOKEN_FUN)
    AS_STR(TOKEN_FOR)
    AS_STR(TOKEN_IF)
    AS_STR(TOKEN_NIL)
    AS_STR(TOKEN_OR)
    AS_STR(TOKEN_PRINT)
    AS_STR(TOKEN_RETURN)
    AS_STR(TOKEN_SUPER)
    AS_STR(TOKEN_THIS)
    AS_STR(TOKEN_TRUE)
    AS_STR(TOKEN_VAR)
    AS_STR(TOKEN_WHILE)
    AS_STR(TOKEN_ERROR)
    AS_STR(TOKEN_EOF)
  default:
    return "Unknown";
  }
#undef AS_STR
}
