Lox: com/craftinginterpreters/lox/Lox.class

com/craftinginterpreters/lox/Lox.class: com/craftinginterpreters/lox/*.java
	javac -g com/craftinginterpreters/lox/*.java

run: com/craftinginterpreters/lox/Lox.class
	java com/craftinginterpreters/lox/Lox

clean: 
	rm com/craftinginterpreters/lox/*.class
