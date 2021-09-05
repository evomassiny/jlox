Lox: com/craftinginterpreters/lox/Lox.class

com/craftinginterpreters/lox/Lox.class: com/craftinginterpreters/lox/*.java
	javac -g com/craftinginterpreters/lox/*.java

GenerateAst: com/craftinginterpreters/tool/GenerateAst.class

com/craftinginterpreters/tool/GenerateAst.class: com/craftinginterpreters/tool/GenerateAst.java
	javac -g com/craftinginterpreters/tool/GenerateAst.java

run: com/craftinginterpreters/lox/Lox.class
	java com/craftinginterpreters/lox/Lox

clean: 
	rm com/craftinginterpreters/lox/*.class
