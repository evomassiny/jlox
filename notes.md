# About
Those are unordered personal notes, written while reading the book.

Those have no value to anyone but me.

# Notes
* embed source span in AST ? Nystrom does it by storing tokens (sometime).
* Statements are distinguished from Expressions, this means:
  * scoping is easier,
  * variable declarations are statements, not expressions, this means we can have special rules for them, for instance in Java:
  ```java
    if (true) int a = 1;  // won't compile
    if (true) { // this is ok
        int b = 1;
    }
  ```
  * in C, **assignment** are **expressions** not **statements**. Which means they produce a value _AND_ have side effect.
* Nystrom calls the `IDENTIFIER -> data` binding "Environment", In toyland I called it "Context".
* **r_value** are expressions that evaluates to Value, **l_value** are expressions that evaluate to binding reference.
* on classes: "every field is a property, but not every property is a field".
  IMO fiels are instance-local data. Properties are Class + instance data.
* setters dont chain: eg `a.b.c = 1` => `a.get("b").get("c").set(1)`
* methods can be bound to an interpreter instance, This is what happens when we 
  use a method, the function is bounded to the class instance: eg, we add "this" to the function environment.
* You cannot `print super`, super is not a variable by itself, `super.IDENTIFIER` is.
* `super` can be embeded in a environment.
* in the bytecode we stores the codes, constants, and line numbers in 3 different arrays.
* the byte code "code" might contains OpCode AND their Operand (voids).
* it is faster to dereference a pointer than to look up an element in an array by index, I'm not sure why though.
* There is a GCC extension which allow jumping to an address contained in a pointer. It's called "computed goto", CPYthon uses it.
* the name of an array is evaluated as the address of its first element. eg:
  ```C
  int array[] = {0, 1};
  if (array == &array[0]) printf("Always true.");
  ```
* to embed mutliple instructions in a MACRO, use a `do { stuff; stuff } while (false)` construct,
  so it works if you append a semicolon and is block scoped.
* `const char * v` means the value pointed by v cannot be mutated,
* `char * const v` means the pointer cannot be mutated,
* `const char * const v` means nor `v` nor `*v` can be mutated.
* the text of a string can be stored in the function stack OR in the code itself, depending on how we define it:
    ```C
    char local[] = "'get_ptr' local";   // array on stack
    char* local = "'get_ptr' local";    // pointer on stack, array in static memory
    ```
* `setjmp()` and `lonjmp()` can emulate exception in C.
* in **C**, `enum`s can be casted into `uint` and vice versa.
* negative indices are valid for C arrays !
* **variadic function** allow you to define a function that takes a dynamic number of arguments, like printf() (`man va_list`)
* hashmaps can either store a linked list of value per bucket, or have an heuristic to find another empty bucket if the target one is full.
  This is called "probing", clox uses "linear" probing: if a bucket is full, try its next neighbour. CPython uses [another open addressing strategy](https://hg.python.org/cpython/file/52f68c95e025/Objects/dictobject.c#l33), it also store the objects in a separated (compact) array from the indices (in a sparse array).
* clox use 'FNV-1a' as its hashing algorithm
* in clox, when declaring a variable, it goes through 3 steps:
  * undefined: before parsing anything related to the variable,
  * defined, after parsing `var a = `
  * declared, after parsing ` = <expression>`
  The distinction between those last 2 states is necessary to handle the edge case of a variable referencing a variable 
  using the same identifier, but defined in an outer scope.
  EG:
  ```
  var a = 1; { var a = a; }
  #                    ^ here 'a' must be evaluated to `1` not Nil.
  ```
* resolving the address of an "else" block in an "if" statement AFTER emyting the JUMP instruction is called **backpatching**. 
* we can store local variables directly into the vm stack, their adrees became their slot index
* function declaration are Function Object literals !
* There s a way to describe "when" the work is done while traversing a recursive function call stack, eg:
  ```
  def pre_order_traversal(count: int):
    if count == 0:
      print("STOP")
    else:
      print("Continue") # work
      pre_order(count - 1)


  def post_order_traversal(count: int):
    if count == 0:
      print("STOP")
    else:
      post_order_traversal(count - 1)
      print("Continue") # work
  ```
* `upvalue`s are closure's bindings to other variable which were defined in outer stackframes. If the variable still lives in its original stack frame,
  we call it **open**, if we had to move it the heap, we call it **closed**.
* While traversing the graph of allaocated/heap objects, we mark them in 3 "colors":
  * **white**: we never reached the node
  * **grey**: we reached the node, and we're currently processing it, or its children.
  * **black**: we processed the node, and its children.
  Those 3 variants end up in any kind of tree traversal, I should use those naming convention from now on.
* the USA says "gray", UK says "grey"
* a **weak reference** is a reference that doesn't protect the referenced object from being garbage collected, eg: in the
  "mark" phase of a "mark and sweep" GC, weak refs are not dereferenced.
* in GC land, **throughput** represents the time `running/(running + GC-ing)`, and **latency** is the longest continuous GC time.
* `class` declaration produces a runtime object, bound to a variable (the class name), this means class types are usual objects.
* class instances' fields are variables, **properties** are variables + **methods**
* methods are "bound" to an object instance, in python, this instance is avaible through the method "__self__" attribute
* `CPython` uses the same trick as `clox` to store its locals: they are stored directly into the stackframe stack.
  This is what python's `LOAD_FAST` and `STORE_FAST` opcodes do, they exchange data between the stack slots and the top of the stack  (`frame->localsplus`).
  Eg: `LOAD_FAST` is exactly equivalent to `OP_GET_LOCAL`, same goes for `STORE_FAST` and `OP_SET_LOCAL`.
* `a % (b^2)` is equivalent to `a & (b^2 -1)` as long as `a` and `b` are uints. This makes sense if you tranpose it to decimal
  `180 % 100` is all the digit after the `1` of `100`.
* casting bytes without doing any kind of value conversion is called **type punning**
