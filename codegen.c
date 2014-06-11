#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "symbolTable.h"
#include "codegen.h"


extern SymbolTable symbolTable;
extern int ARoffset;
void walkTree (FILE *F, AST_NODE *node);
void emitArithmeticStmt (FILE *F, AST_NODE *exprNode);

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
    // xatier: global variables stored in .data segment
    // Ex.
    // .data
    // _result:    .word 0

    AST_NODE *decl = prog->child;

    fprintf(F, ".data\n");
    while (decl != NULL) {
        // XXX: xatier this is wrong because decl->semantic_value.declSemanticValue is not initialized, so everything inside is zero
        // that cause kind == VARIABLE_DECL
        // Because you uncomment the emitPreface in codegen again... I design this func for walktree part
        // So it is definitely decl semantic, if you have paid attention on walk tree part
        if (decl->semantic_value.declSemanticValue.kind == VARIABLE_DECL) {
            AST_NODE *id = decl->child->rightSibling;

            // XXX: xatier: this is an infinite loop, cause you don't change id ...
            // fixed
            while (id != NULL) {
                AST_NODE *dim = id->child;
                int size = 1;

                switch (id->semantic_value.identifierSemanticValue.kind) {
                    case NORMAL_ID:
                        if (id->dataType == FLOAT_TYPE)
                            fprintf(F, "_%s: .float 0.0\n", id->semantic_value.identifierSemanticValue.identifierName);
                        else
                            fprintf(F, "_%s: .word 0\n", id->semantic_value.identifierSemanticValue.identifierName);
                        break;

                    case ARRAY_ID:
                        while (dim) {
                            if (dim->nodeType == CONST_VALUE_NODE)
                                size *= dim->semantic_value.const1->const_u.intval;
                            else if (dim->nodeType == EXPR_NODE)
                                size *= dim->semantic_value.exprSemanticValue.constEvalValue.iValue;

                            dim = dim->rightSibling;
                        }

                        fprintf(F, "_%s: .space %d\n", id->semantic_value.identifierSemanticValue.identifierName, size*4);
                        break;

                    case WITH_INIT_ID:
                        if (id->dataType == FLOAT_TYPE && id->child->nodeType == CONST_VALUE_NODE)
                            fprintf(F, "_%s: .float %f\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.const1->const_u.fval);
                        else if (id->dataType == FLOAT_TYPE && id->child->nodeType == EXPR_NODE)
                            fprintf(F, "_%s: .float %f\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.exprSemanticValue.constEvalValue.fValue);
                        else if (id->child->nodeType == CONST_VALUE_NODE)
                            fprintf(F, "_%s: .word %d\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.const1->const_u.intval);
                        else
                            fprintf(F, "_%s: .word %d\n", id->semantic_value.identifierSemanticValue.identifierName, id->child->semantic_value.exprSemanticValue.constEvalValue.iValue);
                        break;

                    default:
                        break;
                }
            id = id->rightSibling;
            }
        }

        decl = decl->rightSibling;
    }

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

    if (blockNode->child->nodeType != VARIABLE_DECL_LIST_NODE)
        return;

    AST_NODE *decl = blockNode->child->child;
    int total = 0;
    while (decl != NULL) {
        AST_NODE *id = decl->child->rightSibling;

        while (id != NULL) {
            AST_NODE *dim = id->child;
            int size = 1;
            switch (id->semantic_value.identifierSemanticValue.kind) {
                case NORMAL_ID:
                    total += 4;
                    break;

                case ARRAY_ID:

                    while (dim != NULL) {
                        if (dim->nodeType == CONST_VALUE_NODE)
                            size *= dim->semantic_value.const1->const_u.intval;
                        else if (dim->nodeType == EXPR_NODE)
                            size *= dim->semantic_value.exprSemanticValue.constEvalValue.iValue;

                        dim = dim->rightSibling;
                    }
                    total += size*4;
                    break;

                case WITH_INIT_ID:
                    total += 4;
                    break;

                default:
                    break;

            }
            id = id->rightSibling;
        }
        decl = decl->rightSibling;
    }

    ARoffset += total;
    return;
}

void emitAssignStmt (FILE *F, AST_NODE *assignmentNode) {
    _DBG(F, assignmentNode, "assign =");
    //SymbolTableEntry *entry = retrieveSymbol(assignmentNode->child->semantic_value.identifierSemanticValue.identifierName);
    SymbolTableEntry *entry = assignmentNode->child->semantic_value.identifierSemanticValue.symbolTableEntry;

    if(assignmentNode->child->rightSibling->nodeType == EXPR_NODE)
        walkTree(F, assignmentNode->child->rightSibling);
    else
        emitArithmeticStmt(F, assignmentNode->child->rightSibling);

    fprintf(F, "sw      $t0, ($sp)\n");
    fprintf(F, "sub     $sp, $sp, 4\n");

    if (assignmentNode->child->dataType == INT_TYPE) {
        fprintf(F, "lw      $t0, 8($sp)\n");
        if(entry->nestingLevel == 0)
            fprintf(F, "sw      $t0, _%s\n", assignmentNode->child->semantic_value.identifierSemanticValue.identifierName);
        else
            fprintf(F, "sw      $t0, %d($fp)\n", entry->offset);
    }
    else if (assignmentNode->child->dataType == FLOAT_TYPE) {
        if (assignmentNode->child->rightSibling->dataType == INT_TYPE) {
            // assign float <- int
            fprintf(F, "lw      $t0, 8($sp)\n");
            fprintf(F, "mtc1    $t0, $f0\n");
            fprintf(F, "nop");
            if(entry->nestingLevel == 0)
                fprintf(F, "swc1    $f0, _%s\n", assignmentNode->child->semantic_value.identifierSemanticValue.identifierName);
            else
                fprintf(F, "swc1    $f0, %d($fp)\n", entry->offset);
        }
        else if (assignmentNode->child->rightSibling->dataType == FLOAT_TYPE) {
            // assign float <- float
            fprintf(F, "lwc1    $f0, 8($sp)\n");
            if(entry->nestingLevel == 0)
                fprintf(F, "swc1    $f0, _%s\n", assignmentNode->child->semantic_value.identifierSemanticValue.identifierName);
            else
                fprintf(F, "swc1    $f0, %d($fp)\n", entry->offset);

        }
        else {
            printf("invalid assignment! [float <- ?]\n");
            exit(1);
        }
    }
    else {
        printf("invalid assignment! [? <- ?]\n");
        exit(1);
    }

    fprintf(F, "lw      $t0, 4($sp)\n");
    fprintf(F, "addiu   $sp, $sp, 8\n");

    return;
}

void emitIfStmt (FILE *F, AST_NODE *ifNode) {
    _DBG(F, ifNode, "if ( ... )");
    AST_NODE *ifBlock = ifNode->child->rightSibling;

    char ifLabel[10];
    sprintf(ifLabel, "ifl_%d", rand() % 10000);

    if(ifNode->child->nodeType == EXPR_NODE) {
        AST_NODE *temp = ifNode->child->rightSibling;
        ifNode->child->rightSibling = NULL;
        walkTree(F, ifNode->child);
        ifNode->child->rightSibling = temp;
    }
    else
        emitArithmeticStmt(F, ifNode->child);

    fprintf(F, "sw      $t0, ($sp)\n");
    fprintf(F, "sub     $sp, $sp, 4\n");

    // xatier: 4($sp) == the condition
    // jyhsu: i change the push/pop mechanics, so 8 now
    fprintf(F, "lw      $t0, 8($sp)\n");
    fprintf(F, "beqz    $t0, %s_else\n", ifLabel);

    fprintf(F, "lw      $t0, 4($sp)\n");
    fprintf(F, "addiu   $sp, $sp, 8\n");

    _DBG(F, ifBlock, "block {");
    walkTree(F, ifBlock->child);
    _DBG(F, ifBlock, "block }");

    fprintf(F, "j       %s_exit\n", ifLabel);

    fprintf(F, "%s_else:\n", ifLabel);
    if (ifBlock->rightSibling->nodeType != NUL_NODE) {
        // else-if
        if (ifBlock->rightSibling->semantic_value.stmtSemanticValue.kind == IF_STMT) {
            emitIfStmt(F, ifBlock->rightSibling);
        }
        // only else
        else {
            walkTree(F, ifBlock->rightSibling);
        }
    }
    fprintf(F, "%s_exit:\n", ifLabel);
}


void emitWhileStmt (FILE *F, AST_NODE *whileNode) {
    char whileLabel[10];
    sprintf(whileLabel, "whilel_%d", rand() % 10000);
    _DBG(F, whileNode, "while ( ... )");

    fprintf(F, "%s:\n", whileLabel);

    if(whileNode->child->nodeType == EXPR_NODE) {
        AST_NODE *temp = whileNode->child->rightSibling;
        whileNode->child->rightSibling = NULL;
        walkTree(F, whileNode->child);
        whileNode->child->rightSibling = temp;
    }
    else
        emitArithmeticStmt(F, whileNode->child);

    fprintf(F, "lw      $t0, ($sp)\n");
    fprintf(F, "add     $sp, $sp, 4\n");
    fprintf(F, "beqz    $t0, %s_exit\n", whileLabel);
    walkTree(F, whileNode->child->rightSibling);
    fprintf(F, "j       %s\n", whileLabel);
    fprintf(F, "%s_exit:\n", whileLabel);
    return;
}

void emitForStmt (FILE *F, AST_NODE *forNode) {
    _DBG(F, forNode, "for ( ... )");
    return;
}

char *genReturnJumpLabel(AST_NODE *retNode) {
    AST_NODE *parent = retNode->parent;
    while(parent != NULL) {
        if(parent->nodeType == DECLARATION_NODE)
            if(parent->semantic_value.declSemanticValue.kind == FUNCTION_DECL)
                break;
        parent = parent->parent;
    }
    return parent->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
}

void emitRetStmt (FILE *F, AST_NODE *retNode) {
    _DBG(F, retNode, "return ... ;");
    if(retNode->child != NULL) {
        if(retNode->child->nodeType == EXPR_NODE)
            walkTree(F, retNode->child);
        else
            emitArithmeticStmt(F, retNode->child);

        fprintf(F, "lw      $v0, 4($sp)\n");
        fprintf(F, "add     $sp, $sp, 4\n");
        fprintf(F, "j       _end_%s\n", genReturnJumpLabel(retNode));
    }
    return;
}

void emitVarDecl (FILE *F, AST_NODE *declarationNode) {
    _DBG(F, declarationNode, "declare Var ...");
    AST_NODE *id = declarationNode->child->rightSibling;

    while (id != NULL) {
        AST_NODE *dim = id->child;
        int size = 1;
        switch (id->semantic_value.identifierSemanticValue.kind) {
            case NORMAL_ID:
                fprintf(F, "sub     $sp, $sp, 4\n");
                break;

            case ARRAY_ID:

                while (dim != NULL) {
                    if (dim->nodeType == CONST_VALUE_NODE)
                        size *= dim->semantic_value.const1->const_u.intval;
                    else if (dim->nodeType == EXPR_NODE)
                        size *= dim->semantic_value.exprSemanticValue.constEvalValue.iValue;

                    dim = dim->rightSibling;
                }
                fprintf(F, "sub     $sp, $sp, %d\n", size*4);
                break;

            case WITH_INIT_ID:
                if (id->dataType == FLOAT_TYPE && id->child->nodeType == CONST_VALUE_NODE) {
                    fprintf(F, "li.s    $f0, %f\n", id->child->semantic_value.const1->const_u.fval);
                    fprintf(F, "s.s     $f0, ($sp)\n");
                }
                else if (id->dataType == FLOAT_TYPE && id->child->nodeType == EXPR_NODE) {
                    fprintf(F, "li.s    $f0, %f\n", id->child->semantic_value.exprSemanticValue.constEvalValue.fValue);
                    fprintf(F, "s.s     $f0, ($sp)\n");
                }
                else if (id->child->nodeType == CONST_VALUE_NODE) {
                    fprintf(F, "li      $t0, %d\n", id->child->semantic_value.const1->const_u.intval);
                    fprintf(F, "sw      $t0, ($sp)\n");
                }
                else {
                    fprintf(F, "li      $t0, %d\n", id->child->semantic_value.exprSemanticValue.constEvalValue.iValue);
                    fprintf(F, "sw      $t0, ($sp)\n");
                }
                fprintf(F, "sub     $sp, $sp, 4\n");
                break;

            default:
                break;
        }
        id = id->rightSibling;
    }
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
    fprintf(F, "sub   $sp, $sp, 4\n");
    fprintf(F, "sw    $v0, ($sp)\n");
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
    fprintf(F, "sub   $sp, $sp, 4\n");
    fprintf(F, "s.s   $f0, ($sp)\n");
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
    // no need
    int paramtype = actualParameter->dataType;
    char label[10];
    sprintf(label, "l_%d", rand() % 10000);
    switch (paramtype) {
        case INT_TYPE:
            emitArithmeticStmt(F, actualParameter);
            fprintf(F, "li      $v0, 1\n");
            fprintf(F, "lw      $a0, 4($sp)\n");
            fprintf(F, "add     $sp, $sp, 4\n");
            fprintf(F, "syscall\n");
            break;

        case FLOAT_TYPE:
            emitArithmeticStmt(F, actualParameter);
            fprintf(F, "li      $v0, 2\n");
            fprintf(F, "l.s     $f12, 4($sp)\n");
            fprintf(F, "add     $sp, $sp, 4\n");
            fprintf(F, "syscall\n");
            break;

        // Note xatier: there's no double in this homework

        case CONST_STRING_TYPE:
            fprintf(F, "li      $v0, 4\n");
            fprintf(F, "la      $a0, %s\n", label);
            fprintf(F, "syscall\n");
            fprintf(F, ".data\n");
            fprintf(F, "%s: .asciiz %s\n", label, actualParameter->semantic_value.const1->const_u.sc);
            fprintf(F, ".text\n");
            break;

        default:
            // xatier: I hope this won't happen
            _DBG(F, functionCallNode, "wrong type for write()");
            exit(1);
    }
    return;
}

void emitBeforeFunc (FILE *F, AST_NODE *funcDeclNode) {
    _DBG(F, funcDeclNode, "before f( ... )");
    // XXX xatier: .text, function name, prologue sequence here
    char *functionName = funcDeclNode->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
    fprintf(F, ".text\n");
    if(strcmp(functionName, "main") == 0)
        fprintf(F, ".globl main\n");
    fprintf(F, "%s:\n", functionName);
    fprintf(F, "# prologue sequence\n");
    // XXX blah blah blah ...
    fprintf(F, "sw      $ra, 0($sp)\n");
    fprintf(F, "sw      $fp, -4($sp)\n");
    fprintf(F, "add     $fp, $sp, -4\n");
    fprintf(F, "add     $sp, $sp, -8\n");

    fprintf(F, "_begin_%s:\n", functionName);

    return;
}

void emitAfterFunc(FILE *F, AST_NODE *funcDeclNode) {
    _DBG(F, funcDeclNode, "after f( ... )");
    char *functionName = funcDeclNode->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
    fprintf(F, "# epilogue sequence\n");
    fprintf(F, "_end_%s:\n", functionName);
    fprintf(F, "lw      $ra, 4($fp)\n");
    fprintf(F, "add     $sp, $fp, 4\n");
    fprintf(F, "lw      $fp, 0($fp)\n");
    if(strcmp(functionName, "main") == 0) {
        fprintf(F, "li      $v0, 10\n");
        fprintf(F, "syscall\n");
    }
    else
        fprintf(F, "jr      $ra\n");
    // _framesize_of_xxx_function:
    //fprintf(F, ".data\n");
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
    char *functionName = functionCallNode->child->semantic_value.identifierSemanticValue.identifierName;
    SymbolTableEntry *entry = functionCallNode->child->semantic_value.identifierSemanticValue.symbolTableEntry;

    fprintf(F, "jal     %s\n", functionName);
    if(entry->attribute->attr.functionSignature->returnType != VOID_TYPE) {
        fprintf(F, "sw      $v0, ($sp)\n");
        fprintf(F, "sub     $sp, $sp, 4\n");
    }
    return;
}



void emitArithmeticStmt (FILE *F, AST_NODE *exprNode) {
    _DBG(F, exprNode, "expr");


    // jyhsu: make sure it is expr node
    if(exprNode->nodeType == CONST_VALUE_NODE) {
        if(exprNode->dataType == FLOAT_TYPE) {
            fprintf(F, "li.s    $f0, %f\n", exprNode->semantic_value.const1->const_u.fval);
            fprintf(F, "s.s     $f0, ($sp)\n");
        }
        else {
            fprintf(F, "li      $t0, %d\n", exprNode->semantic_value.const1->const_u.intval);
            fprintf(F, "sw      $t0, ($sp)\n");
        }
        fprintf(F, "sub     $sp, $sp, 4\n");
        return;
    }
    else if(exprNode->nodeType == IDENTIFIER_NODE) {
        if(exprNode->dataType == FLOAT_TYPE) {
            if(exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                fprintf(F, "l.s     $f0, _%s\n", exprNode->semantic_value.identifierSemanticValue.identifierName);
            else
                fprintf(F, "l.s     $f0, %d($fp)\n", exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset);

            fprintf(F, "s.s     $f0, ($sp)\n");
        }
        else {
            if(exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                fprintf(F, "lw      $t0, _%s\n", exprNode->semantic_value.identifierSemanticValue.identifierName);
            else
                fprintf(F, "lw      $t0, %d($fp)\n", exprNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset);

            fprintf(F, "sw      $t0, ($sp)\n");
        }
        fprintf(F, "sub     $sp, $sp, 4\n");
        return;
    }
    // xatier: make sure they are binary operations
    if (exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION) {
        AST_NODE *leftOp = exprNode->child;
        AST_NODE *rightOp = leftOp->rightSibling;

        // instruction operands are interger (only support signed integer in the homework)
        if (leftOp->dataType == INT_TYPE && rightOp->dataType == INT_TYPE) {

            // xatier: the basic idea is, put operands in $t0 and $t1,
            //     use $t2 as temp is needed
            //     after that, store the result into $t0

            // load leftOp and rightOp to $f0 and $f1
            if(rightOp->nodeType == IDENTIFIER_NODE) {
                if(rightOp->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                    fprintf(F, "lw      $t1, _%s\n", rightOp->semantic_value.identifierSemanticValue.identifierName);
                else
                    fprintf(F, "lw      $t1, %d($fp)\n", rightOp->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
            }
            else if(rightOp->nodeType == CONST_VALUE_NODE) {
                fprintf(F, "li      $t1, %d\n", rightOp->semantic_value.const1->const_u.intval);
            }
            else {
                fprintf(F, "lw      $t1, 4($sp)\n");
                fprintf(F, "add     $sp, $sp, 4\n");
            }

            if(leftOp->nodeType == IDENTIFIER_NODE) {
                if(leftOp->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                    fprintf(F, "lw      $t0, _%s\n", leftOp->semantic_value.identifierSemanticValue.identifierName);
                else
                    fprintf(F, "lw      $t0, %d($fp)\n", leftOp->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
            }
            else if(leftOp->nodeType == CONST_VALUE_NODE) {
                fprintf(F, "li      $t0, %d\n", leftOp->semantic_value.const1->const_u.intval);
            }
            else {
                fprintf(F, "lw      $t0, 4($sp)\n");
                fprintf(F, "add     $sp, $sp, 4\n");
            }

            // we need a unique lable for jump here
            char eqLabel[10];
            char neLabel[10];
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
                    sprintf(eqLabel, "eql_%d", rand() % 10000);
                    fprintf(F, "bne     $t0, $t1, %s\n", eqLabel);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "j       %sxx\n", eqLabel);
                    fprintf(F, "%s:\n", eqLabel);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "%sxx:\n", eqLabel);
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
                    break;

                case BINARY_OP_NE:
                    sprintf(neLabel, "nel_%d", rand() % 10000);
                    fprintf(F, "beq     $t0, $t1, %s\n", neLabel);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "j       %sxx\n", neLabel);
                    fprintf(F, "%s:\n", neLabel);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "%sxx:\n", neLabel);
                    break;

                case BINARY_OP_GT:
                    fprintf(F, "slt     $t0, $t1, $t0\n");
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
                    printf("Undefined operation occurred\n");
                    exit(1);
                    break;
            }

            //push stack frame
            fprintf(F, "sw      $t0, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");
        }
        // instruction operands are float
        else if (leftOp->dataType == FLOAT_TYPE || rightOp->dataType == FLOAT_TYPE) {
            // xatier: handle int -> float conversion
            // some says that we need a nop stall here to avoid hazard
            if (leftOp->dataType == INT_TYPE) {
                if(leftOp->nodeType == IDENTIFIER_NODE)
                    fprintf(F, "lw      $t0, %d($sp)\n", leftOp->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
                else if(leftOp->nodeType == CONST_VALUE_NODE)
                    fprintf(F, "li      $t0, %d\n", leftOp->semantic_value.const1->const_u.intval);
                else
                    fprintf(F, "lw      $t0, 8($sp)\n");

                fprintf(F, "mtc1    $t0, $f0\n");
                fprintf(F, "nop");
                fprintf(F, "swc1    $f0, 8($sp)\n");
            }
            if (rightOp->dataType == INT_TYPE) {
                if(rightOp->nodeType == IDENTIFIER_NODE)
                    fprintf(F, "lw      $t1, %d($sp)\n", rightOp->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
                else if(rightOp->nodeType == CONST_VALUE_NODE)
                    fprintf(F, "li      $t1, %d\n", rightOp->semantic_value.const1->const_u.intval);
                else
                    fprintf(F, "lw      $t1, 4($sp)\n");

                fprintf(F, "mtc1    $t1, $f1\n");
                fprintf(F, "nop");
                fprintf(F, "swc1    $f1, 4($sp)\n");
            }

            // load leftOp and rightOp to $f0 and $f1
            if(rightOp->nodeType == IDENTIFIER_NODE) {
                if(rightOp->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                    fprintf(F, "lwc1    $f1, _%s\n", rightOp->semantic_value.identifierSemanticValue.identifierName);
                else
                    fprintf(F, "lwc1    $f1, %d($fp)\n", rightOp->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
            }
            else if(rightOp->nodeType == CONST_VALUE_NODE) {
                fprintf(F, "li.s    $f1, %f\n", rightOp->semantic_value.const1->const_u.fval);
            }
            else {
                fprintf(F, "lwc1    $f1, 4($sp)\n");
                fprintf(F, "add     $sp, $sp, 4\n");
            }

            if(leftOp->nodeType == IDENTIFIER_NODE) {
                if(leftOp->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                    fprintf(F, "lwc1    $f0, _%s\n", leftOp->semantic_value.identifierSemanticValue.identifierName);
                else
                    fprintf(F, "lwc1    $f0, %d($fp)\n", leftOp->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
            }
            else if(leftOp->nodeType == CONST_VALUE_NODE) {
                fprintf(F, "li.s    $f0, %f\n", leftOp->semantic_value.const1->const_u.fval);
            }
            else {
                fprintf(F, "lwc1    $f0, 4($sp)\n");
                fprintf(F, "add     $sp, $sp, 4\n");
            }

            // for floating point comparision
            char fcmpl[10];
            sprintf(fcmpl, "fcmpl%d", rand() % 10000);

            switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp) {
                case BINARY_OP_ADD:
                    fprintf(F, "add.s   $f0, $f0, $f1\n");
                    break;

                case BINARY_OP_SUB:
                    fprintf(F, "sub.s   $f0, $f0, $f1\n");
                    break;

                case BINARY_OP_MUL:
                    fprintf(F, "mul.s   $f0, $f0, $f1\n");
                    break;

                case BINARY_OP_DIV:
                    fprintf(F, "div.s   $f0, $f0, $f1\n");
                    break;

                // xatier: for floating point comparison, set $t0 = true ? 1 : 0
                case BINARY_OP_EQ:
                    fprintf(F, "c.eq.s   $f0, $f1\n");
                    fprintf(F, "bc1t     %s_t\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "j       %s_exit\n", fcmpl);
                    fprintf(F, "%s_t:\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "%s_exit:\n", fcmpl);
                    break;

                case BINARY_OP_GE:
                    // xatier: ge = not lt
                    fprintf(F, "c.lt.s   $f1, $f0\n");
                    fprintf(F, "bc1t     %s_t\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "j       %s_exit\n", fcmpl);
                    fprintf(F, "%s_t:\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "%s_exit:\n", fcmpl);
                    break;

                case BINARY_OP_LE:
                    fprintf(F, "c.le.s   $f0, $f1\n");
                    fprintf(F, "bc1t     %s_t\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "j       %s_exit\n", fcmpl);
                    fprintf(F, "%s_t:\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "%s_exit:\n", fcmpl);
                    break;

                case BINARY_OP_NE:
                    // xatier: note, bd1f
                    fprintf(F, "c.eq.s   $f0, $f1\n");
                    fprintf(F, "bc1f     %s_f\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "j       %s_exit\n", fcmpl);
                    fprintf(F, "%s_f:\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "%s_exit:\n", fcmpl);
                    break;

                case BINARY_OP_GT:
                    // xatier: gt = not le
                    fprintf(F, "c.le.s   $f1, $f0\n");
                    fprintf(F, "bc1t     %s_t\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "j       %s_exit\n", fcmpl);
                    fprintf(F, "%s_t:\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "%s_exit:\n", fcmpl);
                    break;

                case BINARY_OP_LT:
                    fprintf(F, "c.lt.s   $f0, $f1\n");
                    fprintf(F, "bc1t     %s_t\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 0\n");
                    fprintf(F, "j       %s_exit\n", fcmpl);
                    fprintf(F, "%s_t:\n", fcmpl);
                    fprintf(F, "addi    $t0, $zero, 1\n");
                    fprintf(F, "%s_exit:\n", fcmpl);
                    break;

                default:
                    printf("Undefined operation occurred\n");
                    exit(1);
            }
            //push stack frame
            switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp) {
                case BINARY_OP_ADD: case BINARY_OP_SUB:
                case BINARY_OP_MUL: case BINARY_OP_DIV:
                    fprintf(F, "s.s     $f0, ($sp)\n");
                    break;

                case BINARY_OP_EQ: case BINARY_OP_GE:
                case BINARY_OP_LE: case BINARY_OP_NE:
                case BINARY_OP_GT: case BINARY_OP_LT:
                    fprintf(F, "sw      $t0, ($sp)\n");
                    break;

                default:
                    printf("Undefined operation occurred\n");
                    exit(1);
            }
            fprintf(F, "sub     $sp, $sp, 4\n");
        }
        else {
            printf("Undefined operation occurred\n");
            exit(1);
        }
    }
    else if (exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION) {
        AST_NODE *operand = exprNode->child;
        if (operand->dataType == INT_TYPE) {
            if(operand->nodeType == IDENTIFIER_NODE) {
                if(operand->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                    fprintf(F, "lw      $t0, _%s\n", operand->semantic_value.identifierSemanticValue.identifierName);
                else
                    fprintf(F, "lw      $t0, %d($fp)\n", operand->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
            }
            else if(operand->nodeType == CONST_VALUE_NODE) {
                fprintf(F, "li      $t0, %d\n", operand->semantic_value.const1->const_u.intval);
            }
            else {
                fprintf(F, "lw      $t0, 4($sp)\n");
                fprintf(F, "add     $sp, $sp, 4\n");
            }

            switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp) {
                case UNARY_OP_POSITIVE:
                    // don't need to to anything
                    break;

                case UNARY_OP_NEGATIVE:
                    fprintf(F, "sub     $t0, $zero, $t0\n");
                    break;

                case UNARY_OP_LOGICAL_NEGATION:
                    fprintf(F, "nor     $t0, $t0, $zero\n");
                    break;

                default:
                    printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
                    break;
            }
            //push
            fprintf(F, "sw      $t0, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");
        }
        else if (operand->dataType == FLOAT_TYPE) {
            if(operand->nodeType == IDENTIFIER_NODE) {
                if(operand->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
                    fprintf(F, "lwc1    $f0, _%s\n", operand->semantic_value.identifierSemanticValue.identifierName);
                else
                    fprintf(F, "lwc1    $f0, %d($fp)\n", operand->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
            }
            else if(operand->nodeType == CONST_VALUE_NODE) {
                fprintf(F, "li.s    $f0, %f\n", operand->semantic_value.const1->const_u.fval);
            }
            else {
                fprintf(F, "lwc1    $f0, 4($sp)\n");
                fprintf(F, "add     $sp, $sp, 4\n");
            }

            switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp) {
                case UNARY_OP_POSITIVE:
                    // skip
                    break;

                case UNARY_OP_NEGATIVE:
                    fprintf(F, "neg.s   $f0, $f0\n");
                    break;

                case UNARY_OP_LOGICAL_NEGATION:
                    // skip
                    break;

                default:
                    printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
                    break;
            }
            //push
            fprintf(F, "s.s     $f0, ($sp)\n");
            fprintf(F, "sub     $sp, $sp, 4\n");
        }
        else {
            printf("Undefined operation occurred\n");
            exit(1);
        }
    }
    else {
        printf("Undefined operation occurred\n");
        exit(1);

    }

    return;
}

void walkTree (FILE *F, AST_NODE *node) {
    // xaiter: what does left mean?
    // jyhsu : leftmost sibling
    AST_NODE *left = node;

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
                        // XXX: xatier: this line will cause an error because
                        //     id->semantic_value.identifierSemanticValue.symbolTableEntry == NULL
                        //  It should not be NULL though...'cause entry is stored when decl...
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
                    // XXX: xatier: NULL here, too
                    enterSymbol(left->child->rightSibling->semantic_value.identifierSemanticValue.identifierName, left->child->rightSibling->semantic_value.identifierSemanticValue.symbolTableEntry->attribute);
                    ARoffset = -4;
                    emitBeforeFunc(F, left);
                    walkTree(F, left->child);
                    emitAfterFunc(F, left);
                }
                break;

            case BLOCK_NODE:
                emitBeforeBlock(F, left);
                openScope();
                walkTree(F, left->child);
                closeScope();
                emitAfterBlock(F, left);
                break;

            case STMT_LIST_NODE:
                walkTree(F, left->child);
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

    FILE *output = fopen("output.s", "w");
    srand(time(NULL));

    if (!output) {
        puts("[-] file open error");
        exit(1);
    }

    //emitPreface(output, prog);
    // xaiter: walk the AST
    walkTree(output, prog);
    // end of walk the AST
    emitAppendix(output, prog);

    fclose(output);
    return;
}
