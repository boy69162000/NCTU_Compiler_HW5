NCTU_Compiler_HW5
=================

NCTU_Compiler_HW5


Run
---

```bash
$ ./parser pattern/func.c
Parsing completed. No errors found.

$ spim
SPIM Version 8.0 of January 8, 2010
Copyright 1990-2010, James R. Larus.
All Rights Reserved.
See the file README for a full copyright notice.
Loaded: /usr/lib/spim/exceptions.s
(spim) load "output.s"
(spim) run
0
1
2
3
4
5
6
7
8
9
(spim) quit

```


Sample output
-------------

```asm
# [At: 11]: start
.data
_k: .word 0
# [At: 4]: before f( ... )
.text
try:
# prologue sequence
sw      $ra, 0($sp)
sw      $fp, -4($sp)
add     $fp, $sp, -4
add     $sp, $sp, -8
_begin_try:
# [At: 11]: block {
# [At: 11]: if ( ... )
# [At: 5]: expr
li      $t1, 10
lw      $t0, _k
slt     $t0, $t0, $t1
sw      $t0, ($sp)
sub     $sp, $sp, 4
sw      $t0, ($sp)
sub     $sp, $sp, 4
lw      $t0, 8($sp)
beqz    $t0, ifl_7894_else
lw      $t0, 4($sp)
addiu   $sp, $sp, 8
# [At: 10]: block {
# [At: 6]: write( ... )
# [At: 6]: expr
lw      $t0, _k
sw      $t0, ($sp)
sub     $sp, $sp, 4
li      $v0, 1
lw      $a0, 4($sp)
add     $sp, $sp, 4
syscall
# [At: 7]: write( ... )
li      $v0, 4
la      $a0, l_982
syscall
.data
l_982: .asciiz "\n"
.text
# [At: 8]: assign =
# [At: 8]: expr
li      $t1, 1
lw      $t0, _k
add     $t0, $t0, $t1
sw      $t0, ($sp)
sub     $sp, $sp, 4
sw      $t0, ($sp)
sub     $sp, $sp, 4
lw      $t0, 8($sp)
sw      $t0, _k
lw      $t0, 4($sp)
addiu   $sp, $sp, 8
# [At: 9]: in f( ... )
jal     try
# [At: 10]: block }
j       ifl_7894_exit
ifl_7894_else:
ifl_7894_exit:
# [At: 11]: block }
# [At: 4]: after f( ... )
# epilogue sequence
_end_try:
lw      $ra, 4($fp)
add     $sp, $fp, 4
lw      $fp, 0($fp)
jr      $ra
# [At: 13]: before f( ... )
.text
.globl main
main:
# prologue sequence
sw      $ra, 0($sp)
sw      $fp, -4($sp)
add     $fp, $sp, -4
add     $sp, $sp, -8
_begin_main:
# [At: 17]: block {
# [At: 14]: assign =
# [At: 14]: expr
li      $t0, 0
sw      $t0, ($sp)
sub     $sp, $sp, 4
sw      $t0, ($sp)
sub     $sp, $sp, 4
lw      $t0, 8($sp)
sw      $t0, _k
lw      $t0, 4($sp)
addiu   $sp, $sp, 8
# [At: 15]: in f( ... )
jal     try
# [At: 16]: return ... ;
# [At: 16]: expr
li      $t0, 0
sw      $t0, ($sp)
sub     $sp, $sp, 4
lw      $v0, 4($sp)
add     $sp, $sp, 4
j       _end_main
# [At: 17]: block }
# [At: 13]: after f( ... )
# epilogue sequence
_end_main:
lw      $ra, 4($fp)
add     $sp, $fp, 4
lw      $fp, 0($fp)
li      $v0, 10
syscall
# [At: 18]: end
```
