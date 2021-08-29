Lox: lox/Lox.class

lox/Lox.class: lox/Lox.java
	javac -g com.craftinginterpreters.lox lox/*.java

run: lox/Lox.class
	java lox/Lox

clean: 
	rm lox/*.class
