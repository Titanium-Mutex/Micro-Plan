# makefile for microplan compiler in C, 2022
# mpc - microplan compiler;  program.mp --> program.rc, program.sym
# mpld -microplan r-code loader; program.rc, program.sym --> program.qc
# mpx - microplan q-code interpreter; execute program.qc with program.sym
# mpq - microplan q-code disassember; program.qc --> disassemble list

all:	mpc mpld mpx mpq
mpc:	mpc.c
	cc -o mpc -g mpc.c
mpld:	mpld.c
	cc -o mpld -g mpld.c
mpx:	mpx.c
	cc -o mpx -g mpx.c
mpq:	mpq.c
	cc -o mpq -g -w mpq.c

