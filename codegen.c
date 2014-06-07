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
    AST_NODE *left = node;    

    while(left != NULL) {
        switch(left->nodeType) {
            case DECLARATION_NODE:
                if(left->semantic_value.declSemanticValue.kind == VARIABLE_DECL) {
                
                }
                else if(left->semantic_value.declSemanticValue.kind == FUNCTION_DECL) {
                
                }
                break;
            case BLOCK_NODE:
                break;
            case STMT_NODE:
                switch(left->semantic_value.stmtSemanticValue.kind) {
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

    if(node->child != NULL)
        walkTree(node->child);

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
