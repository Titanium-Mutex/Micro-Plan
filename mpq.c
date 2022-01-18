/****************************************************************/
/* microplan Q-code disassembler
   mpq.c [-t symfile] progname
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

#define MAXMEM 4096
#define MAXNAMELEN 16
#define MAXNAMES 64

unsigned char qmem[MAXMEM];
char *name[MAXNAMES];
short value[MAXNAMES];

int idp;
int pc;
int code_end, code_start=0;
int ch;
int debug, errval;

FILE *qcode, *symtab, *out;

void error(char *msg)
{  fprintf(stderr, "Q-code Error: %s; errval=0x%x pc=%d\n", msg, errval, pc);
   exit(1); }

void read_qcode(char *qfile)
{ int s;
  struct stat statbuf;
  s=stat(qfile, &statbuf);
  qcode=fopen(qfile, "r");
  fread(qmem, 1, statbuf.st_size, qcode);
  code_end=statbuf.st_size;
  }

unsigned int hexval(char ch)
{ if (isdigit(ch)) return(ch-'0');
  else if (('A'<= ch) && (ch<='F')) return(ch-'A'+10);
  else if (('a'<= ch) && (ch<='f')) return(ch-'a'+10);
  else error("Not hex"); }

unsigned int hexbyte(char ch_high, char ch_low)
{ return((hexval(ch_high) << 4) + hexval(ch_low));}

/*unsigned int hexword(char ch1, ch2, ch3, ch4)
{ return(hexbyte(ch1, ch2)<<8 + hexbyte(ch3, ch4));} */

int read_word(int addr)
{ int v;
  v=((qmem[addr]<<8) | qmem[addr+1]) & 0xffff;
  return(v); }

/****************************************************************/

char *search_callee(int address)
{ int i=0;
  while (i<idp) {
    if (value[i]==address) return(name[i]);
    i++;} 
  return((char *)(-1)); }

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

void dump_qcode(FILE *f)
{ 
  fwrite(qmem, 1, code_end, f); }


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

int qdisasm(FILE *f, int pc, char *label)
{ unsigned char inst, operand1, operand2;
  int type, op, mode, addr, advance;
  static  char line[40];

  inst=qmem[pc]; operand1=qmem[pc+1]; operand2=qmem[pc+2];
  type=(inst & 0xc0)>>6;
  if (type == 0x00) {
    mode=inst & 0x03;
    op=inst & 0x3c;
    fprintf(f, "%-6s ", mnemonic[op]);
    switch (mode) {
    case 0: advance=1; break;
    case 1: advance=2;
            if (operand1 & 0x80) fprintf(f, "local-%d", operand1 & 0x7f);
            else fprintf(f, "global-%d", operand1);   break;
    case 2: advance=2; fprintf(f, "#$%02x", operand1); break;
    case 3: advance=3; fprintf(f, "#$%04x", (operand1<<8) | operand2); break; } }
  else if (type==0x01) { /* CALL, JMP, JF */
    advance=2;	
    op=(inst & 0x30) >>4;
    addr=((inst & 0x0f) <<8) + operand1; 
    if (op==0) fprintf(f, "CALL   %s", search_callee(addr));
    else fprintf(f, "%-6s $%04x", mnemonic_j[op], addr); }
  else if (type==0x02) {
    advance=2;
    op=inst & 03;  
    if (operand1 & 0x80) fprintf(f, "%-6s local-%d", mnemonic_p[op], operand1 & 0x7f);
    else  fprintf(f, "%-6s global-%d", mnemonic_p[op], operand1); }
  else if ((inst & 0x07)==6) { /*PUTS*/
    fprintf(f, "PUTS   \"%s\"", &qmem[pc+1]);
    advance=strlen(&qmem[pc+1])+2; } 
    else { advance=1; fprintf(f, "%-6s", mnemonic_c[inst & 0x07]);}
  fputs( "\n", f); 
  return(advance);
  }

void dump_symtab(FILE *f)
{ int i;
  for (i=0; i<idp; i++) {
    fprintf(f, "%16s  0x%x\n", name[i], value[i]); }
   }
   

/****************************************************************/

void main(int argc, char *argv[])
{ int running=TRUE, inst_count=0;
  char optch, qfilename[64], sfilename[64];
  char *label;
  int  word, nogl, bottom;
 
  debug=0;
  qcode=stdin; symtab=stdin; out=stdout;

  while ((optch=getopt(argc, argv, "di:o:q:t:s:")) != -1) {
     switch(optch) {
       case 'd': debug=1; break;
       case 'i': case 'q': strncpy(qfilename, optarg, 64); qcode=fopen(optarg, "r"); 
       case 'o': out=fopen(optarg, "w"); break;
       case 't': case 's': symtab=fopen(optarg, "r"); break;
       } }

  if (optind<argc) { 
    if (strlen(argv[optind])>=60) { fprintf(stderr, "fname too long"); exit(2);}
    strncpy(sfilename, argv[optind], 64);
    strncat(sfilename, ".sym", 5);
    symtab=fopen(sfilename, "r");
    strncpy(qfilename, argv[optind], 64);
    strncat(qfilename, ".qc", 4);
    /* qcode=fopen(qfilename, "w"); */ }

  read_qcode(qfilename);
  read_symbols(symtab);
  
  pc=0; nogl=0; bottom=MAXMEM;

  fprintf(out, "%04x start      ", pc); /* JMP START */
  pc += qdisasm(out, pc, label);
  while (pc<code_start) { /*global variables */
    word=read_word(pc);
    if (word != 0) {
      fprintf(out, "%04x global-%-3d ARRAY  $%04x, %d\n", pc, nogl, word, (bottom-word)/2);
      bottom = word; }
    else fprintf(out, "%04x global-%-3d VAR  0\n", pc, nogl );
    nogl++;
    pc +=2; }

  while (pc<code_end) {
    label=search_callee(pc);
    fprintf(out, "%04x %-10s ", pc, ((label==(char *)(-1))?"   ":label) );
    if (label != (char *)(-1)) {
      fprintf(out, "ENTER  %d, %d\n", qmem[pc], qmem[pc+1]); 
      pc +=2; }
    else pc += qdisasm(out, pc, label);
    inst_count++;} 

  dump_symtab(out );

  fprintf(out, "%d instructions are disassembled %d ticks\n", inst_count, clock());
  exit(0); }

 
