#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "header.h"
#include "symbolTable.h"

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

    return;
}

// a brief documentation for MIPS instructions
//
// Arthmetic instructions
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
    switch (paramtype) {
        case INT_TYPE:
            fprintf(F, "li    $v0, 1\n");
            fprintf(F, "la    $a0, %s\n", buf);
            fprintf(F, "syscall\n");
            break;

        case FLOAT_TYPE:
            fprintf(F, "li    $v0, 2\n");
            fprintf(F, "la    $f12, %s\n", buf);
            fprintf(F, "syscall\n");
            break;

        // Note xatier: there's no double in this homework

        case CONST_STRING_TYPE:
            fprintf(F, "li    $v0, 4\n");
            fprintf(F, "la    $a0, %s\n", string);
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
    return;
}

void walkTree(AST_NODE *node) {
    // xaiter: what does left mean?
    AST_NODE *left = node;

    // this is a DFS?
    if (node->child != NULL)
        walkTree(node->child);

    while (left != NULL) {
        switch (left->nodeType) {
            case DECLARATION_NODE:
                if (left->semantic_value.declSemanticValue.kind == VARIABLE_DECL) {

                }
                else if (left->semantic_value.declSemanticValue.kind == FUNCTION_DECL) {

                }
                break;

            case BLOCK_NODE:
                break;

            case STMT_NODE:
                switch (left->semantic_value.stmtSemanticValue.kind) {
                    case ASSIGN_STMT:
                        break;
                    case IF_STMT:
                        break;
                    case WHILE_STMT:
                        break;
                    case FOR_STMT:
                        break;
                    case RETURN_STMT:
                        break;
                    case FUNCTION_CALL_STMT:
                        break;
                    default:
                        break;
                }
                break;

            case EXPR_NODE:

                break;

            default:

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

    // end of walk the AST
    emitAppendix(output, prog);

    fclose(output);
    return;
}
