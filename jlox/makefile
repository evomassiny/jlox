SRC_DIR := src
BIN_DIR := bin

PHONY := clean 

Lox: com/craftinginterpreters/lox/Lox.class

${BIN_DIR}/com/craftinginterpreters/lox/Lox.class: ${SRC_DIR}/com/craftinginterpreters/lox/*.java
	mkdir -p ${BIN_DIR}
	javac -sourcepath ${SRC_DIR} -d ${BIN_DIR} ${SRC_DIR}/com/craftinginterpreters/lox/*.java

${BIN_DIR}/com/craftinginterpreters/tool/GenerateAst.class: ${SRC_DIR}/com/craftinginterpreters/tool/GenerateAst.java
	mkdir -p ${BIN_DIR}
	javac -sourcepath ${SRC_DIR} -d ${BIN_DIR} ${SRC_DIR}/com/craftinginterpreters/tool/GenerateAst.java

build: ${BIN_DIR}/com/craftinginterpreters/lox/Lox.class
	@echo Building lox

test: build test.lox
	java -cp ${BIN_DIR} com.craftinginterpreters.lox.Lox test.lox

run: run_generate_ast build
	java -cp ${BIN_DIR} com.craftinginterpreters.lox.Lox

run_generate_ast: ${BIN_DIR}/com/craftinginterpreters/tool/GenerateAst.class
	java -cp ${BIN_DIR} com.craftinginterpreters.tool.GenerateAst ${SRC_DIR}/com/craftinginterpreters/lox/

clean: 
	rm ${BIN_DIR} -rf
