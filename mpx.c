
/****************************************************************/
/* microplan Q-code interpreter and R-code loader
/* specification is mady by S. Tomura and first implementation by T. Chikayama, 1978.
/* C implementation by Toshihiro Matsui, 2021.
/****************************************************************/


#define TRUE (1)
#define FALSE (0)
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#define MAXMEM 4096
#define MAXSTACK 256
#define MAXNAMELEN 16
#define MAXNAMES 64

#define push(x) stack[--sp]=(x)
#define pop() (stack[sp++])

unsigned char qmem[MAXMEM];
short stack[MAXSTACK];
char *name[MAXNAMES];
short value[MAXNAMES];

int idp;
int pc, sp, xreg, yreg, zreg, gb, lb;
int entry_address, code_end, code_start=0;
int ch;
int debug, debug_call, debug_ret, debug_mem, errval;

FILE *qcode, *symtab, *mpin, *mpout;

void error(char *msg)
{  fprintf(stderr, "Q-code Error: %s; errval=0x%x pc=%d sp=%d\n",
	msg, errval, pc, sp );
   exit(1); }

void read_qcode(char *qfile)
{ int s;
  struct stat statbuf;
  s=stat(qfile, &statbuf);
  qcode=fopen(qfile, "r");
  fread(qmem, 1, statbuf.st_size, qcode);
  code_end=statbuf.st_size;
  /*  fprintf(stderr, "read_qcode: %s %x %x\n", qfile, qcode, code_end);
  exit(1); */
  }

unsigned int hexval1(char ch)
{ if (isdigit(ch)) return(ch-'0');
  else if (('A'<= ch) && (ch<='F')) return(ch-'A'+10);
  else if (('a'<= ch) && (ch<='f')) return(ch-'a'+10);
  else error("Not hex"); }

unsigned int hexbyte(char ch_high, char ch_low)
{ return((hexval1(ch_high) << 4) + hexval1(ch_low));}

/*unsigned int hexword(char ch1, ch2, ch3, ch4)
{ return(hexbyte(ch1, ch2)<<8 + hexbyte(ch3, ch4));} */

 
void store_byte(int loc, int val)
{ qmem[loc]=val & 0xff; }

void store_word(int loc, int val)
{ 
  if (debug_mem) fprintf(stderr, "store_word: addr=$%x val=%d\n", loc, val);
  if (debug && (code_start<=loc) && (loc <code_end)) {
	errval=loc; error("bombard into code") ;}
  store_byte(loc, (val >>8) & 0xff);		/*big endian*/
  store_byte(loc+1, val & 0xff); }

int read_word(int addr)
{ int v;
  v=((qmem[addr]<<8) | qmem[addr+1]) & 0xffff;
  if (debug_mem) fprintf(stderr, "read_word: addr=$%x val=%d\n", addr, v);
  return(v); }



/****************************************************************
  complex instructions
/****************************************************************/

char *search_callee(int address)
{ int i=0;
  static char *notfound="not found";
  while (i<idp) {
    if (value[i]==address) return(name[i]);
    i++;} 
  return(notfound); }

void call(int target) /*yreg=target*/
{ int i, ppc=pc, arg;
  char *callee;
  /* fprintf(stderr, "CALL: current pc=$%x sp=$%x lb=$%x\n", pc, sp, lb);  */
  xreg=qmem[yreg];
  for (i=0; i<xreg; i++) stack[sp+i-2]= stack[sp+i];
  arg=stack[sp+xreg-1];
  stack[sp+xreg-1]=pc; stack[sp+xreg-2]=lb;
  lb=sp+xreg-2; xreg=qmem[yreg+1]; sp=lb-xreg;
  pc=yreg+2;
  if (debug_call) fprintf(stderr, "CALL: %s(%d) oldpc=$%x newpc=$%x sp=$%x lb=$%x\n",
	search_callee(yreg), arg, ppc, pc, sp, lb);   }


void retp()
{ int ppc=pc;
  sp=lb+2; pc=stack[lb+1]; lb=stack[lb]; 
  if (debug_ret) fprintf(stderr, "RETP: oldpc=$%x pc=$%x  newlb=$%x \n", ppc, pc, lb);
  }


void retf()
{ int ppc=pc;
  xreg=pop();
  sp=lb+2;
  pc=stack[lb+1];
  lb=stack[lb]; 
  push(xreg);
  if (debug_ret) fprintf(stderr, "RETF: oldpc=$%x pc=$%x return=%d $%x newlb=$%x \n", 
		ppc, pc, xreg, xreg, lb); }

void get_byte()
{ yreg= /*getchar() */ fgetc(mpin);
  push(yreg);}

void put_byte()
{ yreg=pop();
  /*putchar(yreg); */
  fputc(yreg, mpout); }

void put_string()
{ int c;
  while ((c=qmem[pc++]) !=0) fputc(c, mpout); 
  }

/****************************************************************
/* debug utilities
/****************************************************************/

char nextch(FILE *f)
{ ch=fgetc(f); }

void skip_blank(FILE *f)
{ while ((0<ch) && (ch<=' ')) nextch(f); }

char *read_name(FILE *f)
{ char buf[64], *name;
  int i=0;
  skip_blank(f);
  if (ch>0) {
    do { buf[i++]=ch; nextch(f);
       } while ((i<64) && (ch>0) && (isalnum(ch)));
    buf[i++]=0;
    name=malloc(i);
    strcpy(name, buf);
    return(name);}
  else return((char *)(-1));}

int num_digit(char d, int base) /* -1 if error */
{ int val;
  if (isdigit(d)) val=d-'0';
  else if (isalpha(d)) {
    if (islower(d)) val=d-'a'+10; 
    else if (isupper(d)) val=d-'A'+10; 
    else return( -1); 
    if (val<=base) return(val);}
  else return(-1);}

int read_numbody(FILE *f, int base)
{ int val=0, digit;
  
  while ((digit=num_digit(ch, base))>=0) {
    val= val*base+digit; nextch(f); }
  return(val); }    

  
int read_num(FILE *f) /*cannot read - (minux) sign*/
{ int val, base=10;

  skip_blank(f);
  
  if (ch=='0') {  /* base character comes next*/     
    nextch(f);
    if (isdigit(ch)) { /*base8*/
      return(read_numbody(f, 8)); }
    else switch(ch) {
      case 'X': case 'x': nextch(f); return(read_numbody(f, 16)); break;
      case 'O': case 'o': nextch(f); return(read_numbody(f, 8)); break;
      default: error("illegal number base letter"); break; } }
   else if (isdigit(ch)) return(read_numbody(f, 10))	;
   else error("illegal number"); }

void read_symbols(FILE *symfile)
{ idp=0;
  nextch(symfile);
  while ((name[idp]=read_name(symfile)) != (char *)(-1)) {
    value[idp]=read_num(symfile);
    idp++;}
  code_start=value[0];
  fprintf(stderr, "Q-code from 0x%x to 0x%x\n%d symbols are found.\n", 
	code_start, code_end, idp); 
  fclose(symfile); }

void dump_symbols(FILE *f)
{ int i;
  for (i=0; i<idp; i++) {
    fprintf(f, "%16s  0x%x\n", name[i], value[i]); } }

void dump_qcode()
{ FILE *core;
  core=fopen("core","w");
  fwrite(qmem,1, MAXMEM, core); }

void sighandler(int code, int addr)
{ fprintf(stderr, "***signal %d %x\n", code, addr);
  dump_qcode();
  dump_symbols(stderr);
  fprintf(stderr, "***memory dumped to core\n");
  exit(code); }

char *mnemonic[64]={
    "PS",
    "PS var",
    "PS imm8",
    "PS imm16",
    "PSM",
    "PSM var",
    "PSM imm8",
    "PSM imm16",
    "PLM",
    "PLM imm8",
    "PLM imm16",
    "PLM var",
    "EX=",
    "EX= var",
    "EX= imm8",
    "EX= imm16",
    "EX<>",
    "EX<> var",
    "EX<> imm8",
    "EX<> imm16",
    "EX<",
    "EX< var",
    "EX< imm8",
    "EX< imm16",
    "EX>",
    "EX> var",
    "EX> imm8",
    "EX> imm16",
    "EX+",
    "EX+ var",
    "EX+ imm8",
    "EX+ imm16",
    "EX-",
    "EX- var",
    "EX- imm8",
    "EX- imm16",
    "EX OR",
    "EX OR var",
    "EX OR imm8",
    "EX OR imm16",
    "EX*",
    "EX* var",
    "EX* imm8",
    "EX* imm16",
    "EX DIV",
    "EX DIV var",
    "EX DIV imm8",
    "EX DIV imm16",
    "EX MOD",
    "EX MOD var",
    "EX MOD imm8",
    "EX MOD imm16",
    "EX AND",
    "EX AND var",
    "EX AND imm8",
    "EX AND imm16",
    "EX ILL",
    "EX ILL var",
    "EX ILL imm8",
    "EX ILL imm16",
    "EX ILL",
    "EX ILL var",
    "EX ILL imm8",
    "EX ILL imm16"};

static char *mnemonic_j[4]={
    "CALL",
    "JMP",
    "JF",
    "ILLJ"};

static char *mnemonic_p[4]={
    "PULL",
    "INC",
    "DEC",
    ""};

static char *mnemonic_c[8]={
    "RETP",
    "RETF",
    "GET",
    "HALT",
    "PUT",
    "NOT",
    "PUTS",
    "ILLC" };

char *qcode_mnemonic(unsigned char inst)
{ 
  if (inst<64) return(mnemonic[inst]);
  else if ((inst & 0xc0)==0x40)  return(mnemonic_j[(inst & 0x30)>>4]);
  else if ((inst & 0xc0)==0x80)  return(mnemonic_p[inst & 0x03]);
  else return(mnemonic_c[inst & 0x7]); }

char *qdisasm(FILE *f, int pc)
{ unsigned char inst, operand1, operand2;
  int type, op, mode, addr;
  static  char line[40];

  inst=qmem[pc]; operand1=qmem[pc+1]; operand2=qmem[pc+2];
  fprintf(f, " %02x ", inst);
  type=(inst & 0xc0)>>6;
  if (type == 0x00) {
    mode=inst & 0x03;
    op=inst & 0x3c;
    fprintf(f, "%s ", mnemonic[op]);
    switch (mode) {
    case 0: break;
    case 1: if (operand1 & 0x80) fprintf(f, "local-%d", operand1 & 0x7f);
            else fprintf(f, "global-%d", operand1);   break;
    case 2: fprintf(f, "#$%02x", operand1); break;
    case 3: fprintf(f, "#$%04x", (operand1<<8) | operand2); break; } }
  else if (type==0x01) {
    op=(inst & 0x30) >>4;
    addr=((inst & 0x0f) <<8) + operand1; 
    if (op==0) fprintf(f, "CALL %s",  search_callee(addr));
    else fprintf(f, "%s $%04x", mnemonic_j[op], addr); }
  else if (type==0x02) {
    op=inst & 03;  
    if (operand1 & 0x80) fprintf(f, "%s local-%d", mnemonic_p[op], operand1 & 0x7f);
    else  fprintf(f, "%s global-%d", mnemonic_p[op], operand1); }
  else fprintf(f, "%s", mnemonic_c[inst & 0x07]);
  fprintf(f, " sp=%04x\n", sp); }

/****************************************************************/

void main(int argc, char *argv[])
{ int running=TRUE, run_count=0;
  char optch, qfilename[64], sfilename[64];
  int type, inst, mode, operand;
 
  debug=0; debug_call=0; debug_ret=0; debug_mem=0;
  qcode=stdin; mpout=stdout; mpin=stdin;

  while ((optch=getopt(argc, argv, "cmdri:o:t:s:")) != -1) {
     switch(optch) {
       case 'c': debug_call=1; break;
       case 'd': debug=1; break;
       case 'i': mpin=fopen(optarg, "r"); break;
       case 'o': mpout=fopen(optarg, "w"); break;
       case 't': case 's': symtab=fopen(optarg, "r"); break;
       case 'r': debug_ret=1; break;
       case 'm': debug_mem=1; break;
    } }

  if (optind<argc) { 
    if (strlen(argv[optind])>=60) { fprintf(stderr, "fname too long"); exit(2);}
    strncpy(qfilename, argv[optind], 64);
    strncat(qfilename, ".qc", 4);
    /* */
    strncpy(sfilename, argv[optind], 64);
    strncat(sfilename, ".sym", 5);
    symtab=fopen(sfilename, "r"); }

  read_qcode(qfilename);
  read_symbols(symtab);

  signal(SIGSEGV, sighandler);
  pc=0; sp=MAXSTACK; gb=2;

  while (running==TRUE) {
    if (debug) { fprintf(stderr, "0x%04x ", pc); qdisasm(stderr, pc);}
    inst=qmem[pc++];
    type=inst & 0xc0;
    if (type == 0) {  /*type-1*/
      mode= inst & 0x03;
      if (mode==0) yreg=pop(); /*pop*/
      else if (mode==1) {
	operand=qmem[pc++];
	if (operand & 0x80) /*local variable*/ yreg=stack[lb-(operand & 0x7f)];
	else yreg=read_word(gb+(operand <<1)); /*load global*/ }
      else if (mode==2) yreg=qmem[pc++];
      else /*mode==3*/ {
         /* phase-B */
         yreg=(qmem[pc]<<8)+qmem[pc+1]; pc +=2;}
         switch ((inst & 0x3c) >>2) { /* op: 0..15 */
            case 0: /*PS*/ push(yreg); break;
            case 1: /*PSM*/ xreg=pop(); yreg=read_word((xreg<<1)+yreg); push(yreg); break;
            case 2: /*PLM*/ xreg=pop(); zreg=pop(); 
               if (debug) {fprintf(stderr, "PLM: qmem[0x%x]=%d\n", yreg+zreg, xreg);} 
               store_word(yreg+(zreg<<1), xreg); break;
            case 3: /*EX=*/ xreg=pop(); if (xreg==yreg) push(0xffff); else push(0); break;
            case 4: /*EX#*/ xreg=pop(); if (xreg!=yreg) push(0xffff); else push(0); break;		
            case 5: /*EX<*/ xreg=pop(); if (xreg<yreg) push(0xffff); else push(0); break;
            case 6: /*EX>*/ xreg=pop(); if (xreg>yreg) push(0xffff); else push(0); break; 
            case 7: /*EX+*/ xreg=pop(); push(xreg+yreg); break;
            case 8: /*EX-*/ xreg=pop(); push(xreg-yreg); break;
            case 9: /*EXOR*/ xreg=pop(); push(xreg | yreg); break;
            case 10: /*EX**/ xreg=pop(); push(xreg * yreg); break;
            case 11: /*EXDIV*/ xreg=pop(); push((xreg & 0xffff) /yreg); break;
            case 12: /*EXMOD*/ xreg=pop(); push((xreg & 0xffff) % yreg); break; 
            case 13: /*EXAND*/ xreg=pop(); push(xreg & yreg); break; }
      } 
    else if (type==0x40) {  /* type-2 */
	yreg=((inst & 0x0f)<<8) + qmem[pc++];
	switch((inst>>4) & 0x03) {
	case 0: call(yreg); break;
	case 1: pc=yreg; break; /*JP: jump*/
	case 2:  if (pop() ==0) pc=yreg; break; /*JF: jump if false*/ }
	}
    else if (type==0x80) {  /* type-3:  PL, INC, DEC */
      operand=qmem[pc++];
      if ((inst & 0x03)==0) /*PL*/ {
	if (operand & 0x80)
	  stack[lb-(operand & 0x7f)]=pop();
	else store_word(gb+(operand<<1), pop()); }
      else if ((inst & 0x03)==1) /*INC*/ {
	if (operand & 0x80) {
	  xreg=stack[lb-(operand & 0x7f)];
	  stack[lb-(operand & 0x7f)]= ++xreg;
	  push(xreg); }
	else {
	  xreg=read_word(gb+(operand<<1)); 
	  store_word(gb+(operand<<1), ++xreg);
	  if (debug) fprintf(stderr, "INC:0x%x %d\n", gb+(operand<<1), xreg);
	  push(xreg);} }
      else if ((inst & 0x03)==2) /*DEC*/ {
	if (operand & 0x80) {
	  xreg=stack[lb-(operand & 0x7f)];
	  stack[lb-(operand & 0x7f)]= --xreg;
	  push(xreg); }
	else {
	  xreg=read_word(gb+(operand<<1)); 
	  store_word(gb+(operand<<1), --xreg);
	  push(xreg);} } }
    else { /*type-4 */
      switch (inst & 0x07) {
	case 0: retp(); break;
	case 1: retf(); break;
	case 2: get_byte(); break;
	case 3: running=FALSE; break;
	case 4: put_byte(); break;
	case 5: stack[sp]= !stack[sp]; break;
	case 6: put_string(); break;
	}  }
      run_count++;
      /* if (run_count>100) exit(100); */
    if (pc>=MAXMEM) exit(2);
     }  /* while running*/
    fprintf(stderr, "%d instructions are executed in %d ticks\n", run_count, clock());
    dump_qcode();
    exit(0); }

 