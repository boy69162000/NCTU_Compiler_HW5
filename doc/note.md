## requirements

1. Assignment statements
2. Arithmetic expressions
3. Control statements: while, if-then-else
4. Parameterless procedure calls
5. Read and Write I/O calls

## design

I want to try to separate assembly code emitting and AST walking stuffs
functions in AST waking part call functions in code emitting part, pass needed AST nodes to them

RA pool is a manager to stack and registers



ASTWalk
    codeGen(AST_NODE *root)

codeEmit

    emitPreface()                // stuffs before the program body, like .data segments ...
    emitAppendix()               // stuffs after the program body,
    emitBeforeBlock()            // stuffs before a {} block, get info from symbol tables
    emitAfterBlock()             // stuffs after a {} block
    emitAssignStmt()             // emit a assignment statement, [ assign target, source ]
    emitIfStmt()
    emitIfElseStmt()
    emitWhileStmt()
    emitForStmt()
    emitRetStmt()
    emitVarDecl()                // get a variable from RA pool
    emitRead()
    emitWrite()
    emitBeforeFunc()            // stuffs before a function, push EBP, move SP ...
    emitAfterFunc()             // stuffs after a function, restore everything
    emitArithmeticStmt()        // a op b, where op = + - * /

RA pool

    resource allocation, which register is in use
    allocate register and spilling out



