#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "header.h"
#include "symbolTable.h"


// xatier: in order to print better asswembly codes, the length of mnemonic + space should be 8
// for instance:
//
//     add     $d, $s, $t
//     addu    $d, $s, $t
//     addiu   $d, $s, C

// xatier: use this function for debug messages
void _DBG (FILE *F, const AST_NODE *node, const char *msg) {
    if (node)
        fprintf(F, "# [At: %d]: %s\n", node->linenumber, msg);
}

// xatier: nothing here, preserve for the future
void emitPreface (FILE *F, AST_NODE *prog) {
    _DBG(F, prog, "start");
    // XXX xatier: global variables stored in .data segment
    // Ex.
    // .data
    // _result:    .word 0

    AST_NODE *decl = prog->child;

    fprintf(F, ".data\n");
    while(decl != NULL) {
        if(decl->semantic_value.declSemanticValue.kind == VARIABLE_DECL) {
            AST_NODE *id = decl->child->rightSibling;
            while(id != NULL) {
                AST_NODE *dim = id->child;
                int size = 1;
                switch(id->semantic_value.identifierSemanticValue.kind) {
                    case NORMAL_ID:
                        if(id->dataType == FLOAT_TYPE)
                            fprintf(F, "_%s: .float 0.0\n", id->semantic_value.identifierSemanticValue.identifierName);
                        else
                            fprintf(F, "_%s: .word 0\n", id->semantic_value.identifierSemanticValue.identifierName);
                        break;
                    case ARRAY_ID:
                        while(dim) {
                            if(dim->nodeType == CONST_VALUE_NODE)
                                size *= dim->semantic_value.const1->const_u.intval;
                            else if(dim->nodeType == EXPR_NODE)
                                size *= dim->semantic_value.exprSemanticValue.constEvalValue.iValue;

                            dim = dim->rightSibling;
                        }

                        fprintf(F, "_%s: .space %d\n", id->semantic_value.identifierSemanticValue.identifierName, size*4);
                        break;
                    case WITH_INIT_ID:
                        if(id->dataType == FLOAT_TYPE && id->child->nodeType == CONST_VALUE_NODE)
                            fprintf(F, "_%s: .float %f\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.const1->const_u.fval);
                        else if(id->dataType == FLOAT_TYPE && id->child->nodeType == EXPR_NODE)
                            fprintf(F, "_%s: .float %f\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.exprSemanticValue.constEvalValue.fValue);
                        else if(id->child->nodeType == CONST_VALUE_NODE)
                            fprintf(F, "_%s: .word %d\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.const1->const_u.intval);
                        else
                            fprintf(F, "_%s: .word %d\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.exprSemanticValue.constEvalValue.iValue);
                        break;
                    default:
                        break;
                }
            }
        }

        decl = decl->rightSibling;
    }
    fprintf(F, ".text\n");
    return;
}

// a brief documentation for MIPS instructions
//
// Arithmetic instructions
// add    $d, $s, $t              # $d = $s + $t
// addu   $d, $s, $t              # unsigned version of above
// sub    $d, $s, $t              # $d = $s - $t
// subu   $d, $s, $t              # $d = $s - $t
// addi   $t, $s, C               # $t = $s + C (signed)
// addiu  $t, $s, C               # unsigned version of above
// mult   $s, $t                  # LO = (($s * $t) << 32) >> 32
//                                # HI = ($s * $t) >> 32
//                                # yeilds a 64-bit result
// multu  $s, $t                  # unsigned version of above
// div    $s, $t                  # LO = $s / $t
//                                # HI = $s % $t
// divu   $s, $t                  # unsigned version of above
// mfhi   $t0                     # move special register Hi to $t0:   $t0 = Hi
// mflo   $t1                     # move special register Lo to $t1:   $t1 = Lo
//
//
// Logical instructions
// and    $d, $s, $t              # $d = $s & $t
// andi   $t, $s, C               # $t = $s & C
// or     $d, $s, $t              # $d = $s & $t
// ori    $t, $s, C               # $t = $s & C
// xor    $d, $s, $t              # $d = $s ^ $t
// nor    $d, $s, $t              # $d = ~($s | $t)
// slt    $d, $s, $t              # $d = $s < $t
// slti   $t, $s, C               # $d = $s < C
//
//
// Bitwise shift
// sll    $d, $t, shamt           # $d = $t << shamt, shift left logical immediate
// srl    $d, $t, shamt           # $d = $t >> shamt, shift right logical immediate
// sllv   $d, $t, $s              # $d = $t << $s
// srlv   $d, $t, $s              # $d = $t >> $s
//
//
// Data transfer insturctions
//
// lw     $t, C($s)               # $t = Memory[$s + C], load word (4 bytes)
// sw     $t, C($s)               # Memory[$s + C] = $t. store word
//
//
// Branch
//
// beq    $s, $t, C               # branch to C if $s == $t
// bne    $s, $t, C               # branch to C if $s != $t
// j      C                       # jump to C
// jr     $s                      # jump to $s
// jal    C                       # jump and link
//
//
//
// Floating point operations
//
// single precision
//
// add.s  $x, $y, $z
// sub.s  $x, $y, $z
// mul.s  $x, $y, $z
// div.s  $x, $y, $z
//
// double precision
//
// add.d  $x, $y, $z
// sub.d  $x, $y, $z
// mul.d  $x, $y, $z
// div.d  $x, $y, $z
//
// XXX: http://www.ece.lsu.edu/ee4720/2014/lfp.s.html
//
//
// Pseudo instructions
// move   $t, $s                   # $t = $s         add $t, $s, $zero
// clear  $t                       # $t =  0         add $s, $zero, $zero
// not    $t, $s                   # $t = ~$s        nor $t, $r, $zero
// b      Label                    # PC = Label      beq $zero, $zero, Label
// bal    Label                    # branch and link
//
// bgt    $s, $t, Label            # if $s > $t,  PC = Label
// blt    $s, $t, Label            # if $s < $t,  PC = Label
// bge    $s, $t, Label            # if $s >= $t, PC = Label
// ble    $s, $t, Label            # if $s <= $t, PC = Label
// bgtu   $s, $t, Label            # if $s > $t,  PC = Label, unsigned
// bgtz   $s, Label                # if $s > 0,   PC = Label
// beqz   $s, Label                # if $s == 0,  PC = Label
//
//
// Registers
//
// $0       $zero      constant 0
// $1       $at        assembler temporary, reserved by the assembler
// $2-3     $v0-1      function returns and expression evaluation
// $4-7     $a0-3      function arguments
// $8-15    $t0-7      temporaries
// $16-23   $s0-7      saved temporaries
// $24-25   $t8-9      temporaries
// $26-27   $k0-1      reserved for OS kernel
// $28      $gp        global pointer
// $29      $sp        stack pointer
// $30      $fp        frame pointer
// $31      $ra        retuen address
//
//
//
// Program Structure
//
//     Ref. http://logos.cs.uic.edu/366/notes/mips%20quick%20tutorial.htm
//
//
// .data        declares variable names used in program; storage allocated in main memory (RAM)
// format for declarations:
//
// name:   storage_type    value(s)
//
//     create storage for variable of specified type with given name and specified value
//     value(s) usually gives initial value(s);
//     for storage type `.space`, gives number of spaces to be allocated
//
// example
//
//     var1:       .word   3       # create a single integer variable with initial value 3
//     array1:     .byte   'a','b' # create a 2-element character array with elements initialized
//                                 #   to  a  and  b
//     array2:     .space  40      # allocate 40 consecutive bytes, with storage uninitialized
//                                 #   could be used as a 40-element character array, or a
//                                 #   10-element integer array; a comment should indicate which!
// .text        contains program code (instructions)
// ending point of main code should use exit system call
//
//



// xatier: nothing here, preserve for the future
void emitAppendix (FILE *F, AST_NODE *prog) {
    _DBG(F, prog, "end");
    // XXX xaiter: constant string data
    // Ex.
    // .data
    // m2: .asciiz "Enter a number:"
    return;
}

void emitBeforeBlock (FILE *F, AST_NODE *blockNode) {
    _DBG(F, blockNode, "block {");
    return;
}

void emitAfterBlock (FILE *F, AST_NODE *blockNode) {
    _DBG(F, blockNode, "block }");
    return;
}

void emitAssignStmt (FILE *F, AST_NODE *assignmentNode) {
    _DBG(F, assignmentNode, "assign = ");
    return;
}

void emitIfStmt (FILE *F, AST_NODE *ifNode) {
    _DBG(F, ifNode, "if ( ... )");
    return;
}

void emitWhileStmt (FILE *F, AST_NODE *whileNode) {
    _DBG(F, whileNode, "while ( ... )");
    return;
}

void emitForStmt (FILE *F, AST_NODE *forNode) {
    _DBG(F, forNode, "for ( ... )");
    return;
}

void emitRetStmt (FILE *F, AST_NODE *retNode) {
    _DBG(F, retNode, "return ... ;");
    return;
}

void emitVarDecl (FILE *F, AST_NODE *declarationNode) {
    _DBG(F, declarationNode, "declare Var ...");
    return;
}

// xatier: int read(void);
// syscall when
//     $v0 == 5
//
// return value
//     stored in $v0
void emitRead (FILE *F, AST_NODE *functionCallNode) {
    _DBG(F, functionCallNode, "read( ... )");
    fprintf(F, "li    $v0, 5\n");
    fprintf(F, "syscall\n");
    return;
}

// xatier: float fread(void);
// syscall when
//     $v0 == 6
//
// return value
//     stored in $f0
void emitFread (FILE *F, AST_NODE *functionCallNode) {
    _DBG(F, functionCallNode, "fread( ... )");
    fprintf(F, "li    $v0, 6\n");
    fprintf(F, "syscall\n");
    return;
}

// xatier:
// void write(const int);
// syscall when
//     $v0 == 1
//     $a0 = integer to print

// voud write(float);
// syscall when
//     $v0 == 2
//     $f12 = float to print

// void write(const double);
// syscall when
//     $v0 == 3
//     $f12 = double to print
//

// void write(const char *);
// syscall when
//     $v0 == 4
//     $a0 = address of null-terminated string to print
void emitWrite (FILE *F, AST_NODE *functionCallNode) {
    _DBG(F, functionCallNode, "write( ... )");

    // XXX xatier: swtich according to parameter type
    AST_NODE *actualParameter = functionCallNode->child->rightSibling->child;

    // this is a buffer for parameter register
    char *buf = "";
    int paramtype;
    switch (paramtype) {
        case INT_TYPE:
            fprintf(F, "li      $v0, 1\n");
            fprintf(F, "la      $a0, %s\n", buf);
            fprintf(F, "syscall\n");
            break;

        case FLOAT_TYPE:
            fprintf(F, "li      $v0, 2\n");
            fprintf(F, "la      $f12, %s\n", buf);
            fprintf(F, "syscall\n");
            break;

        // Note xatier: there's no double in this homework

        case CONST_STRING_TYPE:
            fprintf(F, "li      $v0, 4\n");
            fprintf(F, "la      $a0, %s\n", buf);
            fprintf(F, "syscall\n");
            break;

        default:
            // xatier: I hope this won't happen
            _DBG(F, functionCallNode, "wrong type for write()");
            exit(1);
    }
    return;
}

void emitBeforeFunc (FILE *F, AST_NODE *functionCallNode) {
    _DBG(F, functionCallNode, "before f( ... )");
    // XXX xatier: .text, function name, prologue sequence here
    char *functionName = "";
    fprintf(F, ".text\n");
    fprintf(F, "%s\n", functionName);
    fprintf(F, "# prologue sequence\n");
    // XXX blah blah blah ...

    fprintf(F, "_begin_%s:\n", functionName);
    return;
}

void emitAfterFunc(FILE *F, AST_NODE *functionCallNode) {
    _DBG(F, functionCallNode, "after f( ... )");
    char *functionName;
    fprintf(F, "# epilogue sequence\n");
    fprintf(F, "_end_%s:\n", functionName);
    // _framesize_of_xxx_function:
    fprintf(F, ".data\n");
    //_framesize_of_main: .word 36
    //
    //
    // XXX: the end of main should be a return statement with syscall exit
    //      li    $v0, 0       # system call code for exit = 0
    //      syscall
    //
    //

    return;
}

void emitFunc (FILE *F, AST_NODE *functionCallNode) {
    _DBG(F, functionCallNode, "in f( ... )");
    return;
}



void emitArithmeticStmt (FILE *F, AST_NODE *exprNode) {
    _DBG(F, exprNode, "expr");

    // xatier: make sure they are binary operations
    if (exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION) {
        AST_NODE *leftOp = exprNode->child;
        AST_NODE *rightOp = leftOp->rightSibling;

        // instruction operands are interger (only support signed integer in the homework)
        if (leftOp->dataType == INT_TYPE && rightOp->dataType == INT_TYPE) {
            // xatier: it's a good idea to use $t0, $t1, $t2 for all arithmetic operations although it's slow XD
            // push $t0, $t1, $t2
            fprintf(F, "sub     $sp, $sp, 4\n");
            fprintf(F, "sw      $t0, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");
            fprintf(F, "sw      $t1, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");
            fprintf(F, "sw      $t2, ($sp)\n");

            // xatier: the basic idea is, put operands in $t0 and $t1,
            //     use $t2 as temp is needed
            //     after that, store the result into $t0

            // XXX: load leftOp and rightOp to $f0 and $f1

            switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp) {
                case BINARY_OP_ADD:
                    fprintf(F, "add     $t0, $t0, $t1\n");
                    break;
                case BINARY_OP_SUB:
                    fprintf(F, "sub     $t0, $t0, $t1\n");
                    break;
                case BINARY_OP_MUL:
                    fprintf(F, "mult    $t0, $t1\n");
                    fprintf(F, "mflo    $t0\n");
                    break;
                case BINARY_OP_DIV:
                    fprintf(F, "div     $t0, $t1\n");
                    fprintf(F, "mflo    $t0\n");
                    break;
                case BINARY_OP_EQ:
                    // XXX: fixme
                    break;
                // greater equal = not less than
                // less equal = not greater than
                case BINARY_OP_GE:
                    fprintf(F, "slt     $t0, $t0, $t1\n");
                    fprintf(F, "xori    $t0, 1\n");
                    break;
                case BINARY_OP_LE:
                    fprintf(F, "slt     $t0, $t1, $t0\n");
                    fprintf(F, "xori    $t0, 1\n");
                case BINARY_OP_NE:
                    // XXX: fixme
                    break;
                case BINARY_OP_GT:
                    fprintf(F, "slt     $t0, $t1, t0\n");
                    break;
                case BINARY_OP_LT:
                    fprintf(F, "slt     $t0, $t0, $t1\n");
                    break;
                case BINARY_OP_AND:
                    fprintf(F, "and     $t0, $t0, $t1\n");
                    break;
                case BINARY_OP_OR:
                    fprintf(F, "or      $t0, $t0, $t1\n");
                    break;
                default:
                    fprintf(F, "# undefined operation occurred\n");
                    exit(1);
                    break;
            }

            // XXX: move $t0 to another result holder

            // pop $t0, $t1, $t2
            fprintf(F, "lw      $t2, ($sp)\n");
            fprintf(F, "addiu   $sp, $sp, 4\n");
            fprintf(F, "lw      $t1, ($sp)\n");
            fprintf(F, "addiu   $sp, $sp, 4\n");
            fprintf(F, "lw      $t0, ($sp)\n");
            fprintf(F, "addiu   $sp, $sp, 4\n");
        }
        // instruction operands are float
        else if (leftOp->dataType == INT_TYPE && rightOp->dataType == FLOAT_TYPE) {
            // push $f0, $f1, $f2
            fprintf(F, "swc1    $f0, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");
            fprintf(F, "swc1    $f1, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");
            fprintf(F, "swc1    $f2, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");

            // XXX: load leftOp and rightOp to $f0 and $f1

            // XXX: switch case here
            // XXX: http://www.ece.lsu.edu/ee4720/2014/lfp.s.html
            // add.s  $x, $y, $z
            // sub.s  $x, $y, $z
            // mul.s  $x, $y, $z
            // div.s  $x, $y, $z

            // pop $f0, $f1, $f2
            fprintf(F, "lwc1    $f2, ($sp)\n");
            fprintf(F, "addiu   $sp, $sp, 4\n");
            fprintf(F, "lwc1    $f1, ($sp)\n");
            fprintf(F, "addiu   $sp, $sp, 4\n");
            fprintf(F, "lwc1    $f0, ($sp)\n");
            fprintf(F, "addiu   $sp, $sp, 4\n");
        }
        else {
            fprintf(F, "# undefined operation occurred\n");
            exit(1);
        }
    }

    return;
}

void walkTree(FILE *F, AST_NODE *node) {
    // xaiter: what does left mean?
    // jyhsu : leftmost sibling
    AST_NODE *left = node;
    int ofst = offset;   // xatier: table offset?

    // this is a DFS? // actually not quite so

    while (left != NULL) {
        switch (left->nodeType) {
            case VARIABLE_DECL_LIST_NODE:
                if(symbolTable.currentLevel == 0)
                    emitPreface(F, left);
                else
                    walkTree(F, left->child);
                break;
            case DECLARATION_NODE:
                if (left->semantic_value.declSemanticValue.kind == VARIABLE_DECL) {
                    AST_NODE *id = left->child->rightSibling;

                    //walkTree(left->child);
                    while(id != NULL) {
                        enterSymbol(id->semantic_value.identifierSemanticValue.identifierName, id->semantic_value.identifierSemanticValue.symbolTableEntry->attribute);

                        id->semantic_value.identifierSemanticValue.symbolTableEntry->offset = ARoffset;
                        if(id->semantic_value.identifierSemanticValue.kind == NORMAL_ID) {
                            ARoffset -= 4;
                        }
                        else if(id->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
                            AST_NODE *dim = id->child;
                            int size = 1;
                            while(dim) {
                                if(dim->nodeType == CONST_VALUE_NODE)
                                    size *= dim->semantic_value.const1->const_u.intval;
                                else if(dim->nodeType == EXPR_NODE)
                                    size *= dim->semantic_value.exprSemanticValue.constEvalValue.iValue;

                                dim = dim->rightSibling;
                            }
                            ARoffset -= size*4;
                        }

                        id = id->rightSibling;
                    }
                    emitVarDecl(F, left);
                }
                else if (left->semantic_value.declSemanticValue.kind == FUNCTION_DECL) {
                    enterSymbol(left->child->rightSibling->semantic_value.identifierSemanticValue.identifierName, left->child->rightSibling->semantic_value.identifierSemanticValue.symbolTableEntry->attribute);
                    emitBeforeFunc(F, left);
                    walkTree(F, left->child);
                    emitAfterFunc(F, left);
                    ARoffset = 0;
                }
                break;

            case BLOCK_NODE:
                emitBeforeBlock(F, left);
                openScope();
                walkTree(F, left->child);
                closeScope();
                emitAfterBlock(F, left);
                break;

            case STMT_NODE:
                switch (left->semantic_value.stmtSemanticValue.kind) {
                    case ASSIGN_STMT:
                        emitAssignStmt(F, left);
                        break;
                    case IF_STMT:
                        emitIfStmt(F, left);
                        break;
                    case WHILE_STMT:
                        emitWhileStmt(F, left);
                        break;
                    case FOR_STMT:
                        emitForStmt(F, left);
                        break;
                    case RETURN_STMT:
                        emitRetStmt(F, left);
                        break;
                    case FUNCTION_CALL_STMT:
                        if(strcmp(left->child->semantic_value.identifierSemanticValue.identifierName, "read") == 0)
                            emitRead(F, left);
                        else if (strcmp(left->child->semantic_value.identifierSemanticValue.identifierName, "fread") == 0)
                            emitFread(F, left);
                        else if (strcmp(left->child->semantic_value.identifierSemanticValue.identifierName, "write") == 0)
                            emitWrite(F, left);
                        else
                            emitFunc(F, left);
                        break;
                    default:
                        break;
                }
                break;

            case EXPR_NODE:
                walkTree(F, node->child);
                emitArithmeticStmt(F, node);
                break;

            default:

                walkTree(F, node->child);
                break;
        }

        left = left->rightSibling;
    }

    return;
}

void codeGen(AST_NODE *prog) {

    FILE *output = fopen("w", "output.s");

    if (!output) {
        puts("[-] file open error");
        exit(1);
    }

    emitPreface(output, prog);
    // XXX xaiter: walk the AST
    walkTree(output, prog);
    // end of walk the AST
    emitAppendix(output, prog);

    fclose(output);
    return;
}
