/****************************************************************/
/* microplan R-code loader
/* specification is defined by S. Tomura and
/* the first implementation by T. Chikayama, 1978.
/* C implementation by Toshihiro Matsui, 2021.
/* Copyright Toshihiro Matsui, 2021.
/* The author grants any use of this program for any purposes to anyone.
/*
/* This loader is intended to be used with mpc and mpx.
/* Microplan program source is compiled by mpc and the generated R-code
/* should be given to thie mpld to produce Q-code and the accompanying
/* csymbol table. 
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


#define MAXMEM 4096
#define MAXSTACK 256
#define MAXNAMELEN 16
#define MAXNAMES 64

unsigned char qmem[MAXMEM];
unsigned short stack[MAXSTACK];
char *name[MAXNAMES], defined[MAXNAMES];
short value[MAXNAMES];

int lc, sp, array_bottom, idp;
int entry_address, code_end, line_count;
int ch;
int debug;

FILE *rcode, *qcode, *symtab; 

void error(char *msg)
{  fprintf(stderr, "R-code loader Error: %s; ch=0x%x lc=%d idp=%d sp=%d\n",
	msg, ch, lc, idp, sp);
   exit(1); }


char getch(FILE *port)
{ do {
    ch=fgetc(port);
    if (ch=='\n') line_count++;} while(ch<' ');
  return(ch & 0x7f); }

unsigned int hexval1(char ch)
{ if (isdigit(ch)) return(ch-'0');
  else if (('A'<= ch) && (ch<='F')) return(ch-'A'+10);
  else if (('a'<= ch) && (ch<='f')) return(ch-'a'+10);
  else error("Not hex"); }

unsigned int hexbyte(char ch_high, char ch_low)
{ return((hexval1(ch_high) << 4) + hexval1(ch_low));}

/*unsigned int hexword(char ch1, ch2, ch3, ch4)
{ return(hexbyte(ch1, ch2)<<8 + hexbyte(ch3, ch4));} */

unsigned int rdhexbyte()
{ char ch1, ch2;
  ch1=getch(rcode);
  ch2=getch(rcode);
  return(hexbyte(ch1, ch2));}

unsigned int rdhexword()
{ return((rdhexbyte() <<8) + rdhexbyte()); 
  }
 
void store_byte(int loc, int val)
{ qmem[loc]=val & 0xff; }

void store_word(int loc, int val)
{ store_byte(loc, (val >>8) & 0xff);		/*big endian*/
  store_byte(loc+1, val & 0xff); }

void load_byte(int val)
{ qmem[lc++]= val & 0xff; }

void load_word(int val) /*big endian*/
{ load_byte(val>>8); load_byte(val); }

int read_word(int addr)
{ return((qmem[addr]<<8 | qmem[addr+1]) & 0xffff); }


void read_name(char *buf)
{ int count=0;
  while ((ch=getch(rcode)) !=' ') {
    *buf++=ch;
    count++;
    if (count>=MAXNAMELEN-1) error("name too long");
    }
  *buf=0;
}

int search_name(char *buf)
{ int i=0;
  while (i<idp) {
    if (strcmp(buf, name[i])==0) return(i);
    i++;}
  return(-1); }

/****************************************************************
  command processors
/****************************************************************/

void make_array()
{ int size;
  size=rdhexword();
  array_bottom -= size;
  if (debug) printf("array: addr=0x%x size=%d\n", array_bottom, size);
  load_word(array_bottom); }

void call()
{ char namebuf[MAXNAMELEN];
  int  index, address;

  read_name(namebuf);
  if (debug) fprintf(stderr, "#%s %d\n", namebuf, strlen(namebuf));
  if ((index=search_name(namebuf)) >= 0 ) { /*the name appeared before*/
    address=value[index];
    if (defined[index]) load_word(0x4000 | (address & 0xfff)); /*embed call inst*/
    else { /*undefined*/
      value[index]=lc;	/*new link*/
      load_word(address); /*load the previous link at lc*/
      }
    }
   else {  /*the first time to see the label; register in the name tab and link*/
     if (debug) fprintf(stderr, "forward call: idp=%d lc=0x%x\n", idp, lc); 
     name[idp]=strcpy(malloc(strlen(namebuf)+1), namebuf);
     defined[idp]=FALSE;
     value[idp]=lc;
     load_word(-1);  /*stop mark*/
     idp++;  }
  }

void def_label()
{ char namebuf[MAXNAMELEN];
  int index, address, next_address;

  read_name(namebuf);
  if (debug) fprintf(stderr, "deflabel: $name=%s %d\n", namebuf, strlen(namebuf));
  if ((index=search_name(namebuf)) >= 0 ) { /*call appeared first*/
    defined[index]=TRUE;
    address=value[index]; /*it should have a link*/
    value[index]=lc;
    while (address!=0xffff) { /* link is alive */
       next_address=read_word(address);
      if (debug) fprintf(stderr, "def: addr=%x next=%x val=$%x\n", address, next_address, value[index]); 
       store_word(address, 0x4000 | (lc & 0xfff));
       address=next_address; }
    }
  else { /* defined first*/
    name[idp]=strcpy(malloc(strlen(namebuf)+1), namebuf);
    value[idp]=lc;
    defined[idp]=TRUE;
    idp++; /*lc += 2;*/ }
  }  
    
void pushlc()
{ stack[sp--]=lc; 
  if (sp<0) error("stack overflow"); }

void popref()
{ int address;
  address=stack[++sp];
  store_word(address, ((qmem[address] & 0xf0) <<8 ) + (lc & 0xFff)); }

void popdef_jp()
{ int address;
  address=stack[++sp];
  load_word(0x5000 | address); }

void popdef_jf()
{ load_word(0x6000 | stack[++sp]); }

void jump()
{ load_word(0x5000); }

void jump_false()
{ load_word(0x6000); }

void exchange()
{ int val_top, val_second;
  val_top=stack[++sp];
  val_second=stack[++sp];
  stack[sp--]=val_top;
  stack[sp--]=val_second; }

void entry_point()
{ entry_address=lc; }   


void end_rcode()
{ code_end=lc; }

/****************************************************************
/* debug utilities
/****************************************************************/

void show_memory()
{ }

void disasm()
{}

void dump_qcode(FILE *f)
{ 
  fwrite(qmem, 1, code_end, f); }


void dump_symtab(FILE *f)
{ int i;
  for (i=0; i<idp; i++) {
    fprintf(f, "%16s  0x%x\n", name[i], value[i]); }
  fprintf(f, "%16s  0x%x\n", "main", entry_address); }
   

/****************************************************************/

void main(int argc, char *argv[])
{ int running=TRUE;
  int i;
  char optch, ch1, ch2, rfilename[64], qfilename[64], sfilename[64];

  rcode=stdin; qcode=stdout; symtab=stdout;

  while ((optch=getopt(argc, argv, "di:o:q:r:t:s:")) != -1) {
     switch(optch) {
    case 'd': debug=1; break;
    case 'i': case 'r': rcode=fopen(optarg, "r"); break;
    case 'o': case 'q': qcode=fopen(optarg, "w"); break;
    case 't': case 's':  symtab=fopen(optarg, "w"); break;
    } }

  if (optind<argc) { 
    if (strlen(argv[optind])>=60) { fprintf(stderr, "fname too long"); exit(2);}
    strncpy(rfilename, argv[optind], 64);
    strncat(rfilename, ".rc", 4);
    rcode=fopen(rfilename, "r");
    strncpy(sfilename, argv[optind], 64);
    strncat(sfilename, ".sym", 5);
    symtab=fopen(sfilename, "w");
    strncpy(qfilename, argv[optind], 64);
    strncat(qfilename, ".qc", 4);
    qcode=fopen(qfilename, "w"); }


  line_count=1;
  lc=0; sp= MAXSTACK-1; idp=0;
  array_bottom=MAXMEM;
  code_end=0;

  while (running) {
    ch=getch(rcode);
/*    if (debug) {
	fprintf(stderr, "line=%d lc=%d idp=%d sp=%d ch=%c 0x%x\n", line_count, lc, idp, sp, ch, ch); }
*/
    if (isxdigit(ch)) {
      ch1=ch; ch2=getch(rcode);
      load_byte(hexbyte(ch1, ch2));}
    else switch(ch) {
    case '"': make_array(); break;
    case '#': call(); break;
    case '$': def_label(); break;
    case '*': pushlc(); break;
    case '/': popref(); break;
    case '+': popdef_jp(); break;
    case '-': popdef_jf(); break;
    case '(': jump(); break;
    case ')': jump_false(); break;
    case '%': exchange(); break;
    case '!': entry_point(); break;
    case '.': end_rcode(); running=FALSE;break;
    case 0x0d: case 0x0a: line_count++; break;
    default: error("unknown R-code"); break;
    }   };
  for (i=0; i<idp; i++) {
    if (defined[i]==FALSE) fprintf(stderr, "Undefined label: %s ref=$%x\n",
				   name[i], value[i]);}
  fprintf(stderr, "R code loaded. size=0x%x\n", lc);
  dump_qcode(qcode); 
  dump_symtab(symtab);}

 
