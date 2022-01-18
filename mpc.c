/****************************************************************
/* Micro PLAN
/* Original by S. Tomura
/* Recoded in C, microplan.c, by Toshihiro Matsui, December, 2021
/* % microplan -i <source> -o <r-code> -t <compiler-nametable>
/* Default source is stdin, r-code is stdout, and the compiler-nametable
/* is assumed to be prepended before the source code.
/****************************************************************/

#define TRUE (1)
#define FALSE (0)
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

void pcall();
void expression();
void factor();
void subscript(); 

short int idtab[255];
short int  ch,sy,val,psmode,psval, idp, idq, idr, idn, nogl, nolo,
	noid, idps, funcp, line, chcnt, outcnt, rcodecnt;
short int debug=0;
FILE *source, *rcode, *comptab;
		
void putch(short ch)
{  if (ch)  {
      rcodecnt++;
      fputc(ch, rcode);
      if (++outcnt ==80) {
        /*fputc(0x0d, rcode);*/ fputc(0x0a, rcode);
	outcnt= 0; } }}


void pnhx1(short n)
{   if (n<10) putch(n+48); else putch((n+55) & 0x7f); }

void pnhx2(short n)
{ pnhx1((n/16)& 0x0f); pnhx1(n & 0x0f) ; };

void pnhx4(short n)
   {
      pnhx2((n & 0xffff)/256 &  0xff);
      pnhx2(n & 0xff) ; };
      
void clearps()
{  if ( psmode)  {
      if (psmode==3) { 
         pnhx2(3); pnhx4(psval); }
      else {
         pnhx2(psmode);
         pnhx2(psval) ; };
      psmode=0; }; }

void genex(short op)
{  if (psmode) {
      if (psmode==3) {
        pnhx2(op+3);
        pnhx4(psval)  ; }
      else {
        pnhx2(op+psmode); pnhx2(psval); }; 
      psmode=0
      ; }
   else pnhx2(op); }

void gencode1(short op)
   {  clearps(); pnhx2(op); };
     
void gencode2(short op,short val)
   {
      clearps(); pnhx2(op); pnhx2(val);
      };
      
void genps(short mode, short val)
   {
      clearps();
      psmode= mode; psval= val; };

void error(short n) 
   {
      putchar('\n');
      outcnt= 0; 
      puts("err");
      putchar(n+42);
      putchar(' '); 
      pnhx4(line);
      putchar(' '); 
      pnhx2(chcnt);
      putchar(' ');
      pnhx2(sy);
      putchar(' ');
      pnhx4(val);
      putchar(' ');
      putchar(ch);
      putchar('\n');
      /*halt(); */
      exit(1); };

short int  isletter(short c)
{   return('@'<c) && (c<'['); } 

/*short int isdigit()
{   return  ('/' <ch) && (ch< ':'); } */
   
void getch(FILE *in)
{
   chcnt= ++chcnt ;
   do {
      ch= fgetc(in) & 0x7f;
      if (debug) fputc(ch, stderr);
      if ( ch=='\n' || ch=='\t') {
         line= ++line ;
         chcnt= 0;
         ch= ' ' ; }
      } while (!((0x1f<ch) && (ch<0x60)));  } 

void rdhx(FILE *in)
 {
      val= 0;
      while ((isletter(ch) | isdigit(ch)) && (ch<'G')) 
         {
            if (ch>'@')  val= val*16+ch-55;
            else   val= val*16+ch-'0';
            getch(in); }}

void insymbol()
{  short int  i,  j, l;
   do {
         while (ch==' ') getch(source);
         if (isdigit(ch)) {
            sy= 2;
            val= 0;
            do {
               val= val*10+ch-'0';
               getch(source);
               }  while (isdigit(ch)); } 
         else {
            l= 1;
            idq= idp;
            if (isletter(ch)) {
               do { if (++l  & 1) idtab[idq]= idtab[idq]+ch;
                    else if (idq<254) idtab[++idq ]= ch*256;
                    else error(23);
                    getch(source);
                    } while (isletter(ch) | isdigit(ch));
                 idtab[idq]= idtab[idq] + 0x8000;
                 l= l/2;}
            else {
                  idtab[++idq ]= ch*256+ 0x8000;
                  getch(source); };
            idr= idp;
            idn= noid;
            do {	
               do {
		  j=idr;
                  while (idtab[--idr]>=0) ;
                  } while (!((j-idr==l) | (--idn ==2)));
                i= idp;
                j= idr;		
                while (idtab[++i]==idtab[++j] && i<=idq) { };
                /* repeat  until (idtab[++i ] != idtab[++j ] | (i>idq) */
                } while ( (i<= idq)  &&  (idn!=2)) ;
           if ( idn==2) {
              sy= 0;
              idr= idp ; }
         else if (idn<43) sy= idn;
         else if (idn==43) {
            sy=2; val= ch;
            getch(source) ; }
         else  if ( idn==44 )  {
		    sy= 2; rdhx(source); }
		 else if (idn==45) { 
		    while (ch != '%') getch(source);
		    getch(source);
		    sy= 45 ; }
		 else {
		    sy= 1;
                    if ( idn<nogl )  val= idn-46;
                    else if (idn<nolo) val= idn-nogl+ 0x81;
                    else sy= 0; }
	 ; } }
     while (sy == 45);
    /* printf("sy=%d  val=%d idn=%d \n", sy, val, idn);  */ }; 
     
void check(short s)
{   if (sy==s) insymbol(); else error(s); }

   
short int but(short s)
{ if (sy==s) { insymbol();return FALSE; } else return TRUE;}

void pname(short r) 
{  short x;
   do {
      x=  idtab [++r ]; /*printf("pname %d %x ", r, x);*/
      putch(((x & 0x7f00) / 0x100) );
      putch(x & 0x7f);
      } while (x>=0);
   putch(' ') ; };
 

void factor()
{  short syp, valp;
   if (sy) { syp= sy;
      valp= val; insymbol();
      if ( syp==2 )
         if (valp & 0xff00) genps(3,valp); else genps(2,valp);
      else if (syp==36) {
         expression();
         check(37) ; }
      else if (syp/2 ==8) { 
         check(36);
         if (sy != 1) error(26); gencode2(syp+113, val); insymbol();
            check(37) ; }
      else if (syp==25) { factor();
         gencode1(0xc5) ; }
      else if (syp==26) { check(36); /*get()*/
         check(37); gencode1(0xc2) ; }
      else  if ( syp==1 )
         if (sy != 39 )  genps(1, valp);
         else {
            subscript(); gencode2(0x05,valp) ; }
      else error(29) ; }
   else pcall(); }
   
void  term()
{  short  op;
   factor();
   while ( sy / 4==3  )  {	/* mutiplying operator */
         op= 4*sy-8;
         insymbol();
         factor();
         genex(op); }}
         
void simple()	/*<simple expression>*/
{  short op;
     if (sy/3==1) genps(2,0); else term();
     while ( sy / 3==1   )  {
         /*<adding operator>*/
         op= 4*sy+16; insymbol(); term(); genex(op); } };
         
void  expression()  /*<expression>*/
{  short op;
   simple();
   if ( sy/4==2 )  {  /*<relational operator>*/
      op= 4*sy-20;
      insymbol();
      simple();
      genex(op); } };
      
void subscript()  
  { insymbol(); expression(); check(40); };

void pcall()  
{  short p, r, n; 
   p= idp; r= idr; n= noid;
   if (idp==idr) { idp= idq; noid= ++noid; };
   insymbol();
   check(36);
   if (sy != 37) do {expression();} while (but(29)==FALSE);  /*comma*/
   check(37); /* right paren */
   clearps();
   putch('#');
   pname(r); 
   noid= n;
   idp= p; };

void statement() 
{  short syp,valp;
   if (sy / 4 != 8 )
      if (sy) { /*defined id | keywords*/
         syp= sy; valp= val;
         insymbol();
         if ( syp==1 ) /*defined id --> assignment*/
            if ( sy==41 )  {
              insymbol(); expression(); gencode2(0x80,valp) ; }
            else if (sy==39) {
               subscript(); check(41);
               expression();
               gencode2(0x09,valp) ; }
            else  error(28);
         else if (syp==20 )  {  /* begins compound statement */
            do { statement();}  while (but(35)==FALSE);  /* ; */
            check(32); } /*end */
         else if (syp==21) { expression(); /*if*/
            clearps(); putch('*');
            putch(')');
            check(30); statement();
            if (sy==34) { /*ELSE*/
               putch('*');  putch('(');  putch('%');  putch('/'); insymbol();
               statement() ; };
            putch('/') ; }
         else if (syp==22) {  /*WHILE*/
            putch('*'); expression(); clearps(); check(31); putch('*');
            putch(')'); statement(); putch('%');  putch('+'); putch('/') ; }
         else if (syp==23) {  /*REPEAT*/
            putch('*');
            do { statement();} while (but(35)==FALSE);
	    check(33);  expression(); clearps(); putch('-'); }
         else if (syp==24) { expression(); gencode1(0xc1);} /*RETURN*/
         else if (syp==27) { check(36); /*PUT*/
            do {
               if (sy==42) { gencode1(0xc6);
                  while (ch != '\'') {
                     if ( ch=='\'' ) getch(source);  /*escape*/
                       pnhx2(ch); getch(source); };
                  pnhx2(0); getch(source); insymbol() ; }
               else { expression() ; gencode1(0xC4) ; } 
               } while (but(29)==FALSE);
            check(37); }
         else if (syp==28) { /*HALT*/
            check(36); check(37); gencode1(0xc3) ; }
         else error(28) ; }  /* if (sy) */
      else pcall(); }

void declare(short s, short b)
  {
    if (sy != s) do {
      if ( (sy<2) && (idn<nogl))  {
        if ( (noid-nogl)> 126 )  error(24);
        nolo= ++noid;
        idp= idq; 
        insymbol();
        if ( b )  {
          check(39);
          if (sy != 2) error(25);
          putch('\"');
          pnhx4(++val  *2); insymbol(); check(40); } }
      else error(27);
      } while (but(29)==FALSE);
  check(s) ; };

void main(int argc, char *argv[])
{ int i, x;
  char optch, sfilename[64], rfilename[64];

  source=stdin; rcode=stdout; comptab=stdin;

  while ((optch=getopt(argc, argv, "o:r:i:t:d")) != -1) {
     switch(optch) {
    case 'd': debug=1; break;
    case 'i': source=fopen(optarg, "r"); break;
    case 'o': case 'r': rcode=fopen(optarg, "w"); break;
    case 't': comptab=fopen(optarg, "r"); break;
    } }
  if (optind<argc) { 
    if (strlen(argv[optind])>=60) { fprintf(stderr, "fname too long"); exit(2);}
    strncpy(sfilename, argv[optind], 64);
    strncat(sfilename, ".mp", 4);
    source=fopen(sfilename, "r");
    strncpy(rfilename, argv[optind], 64);
    strncat(rfilename, ".rc", 4);
    rcode=fopen(rfilename, "w"); }

/*  idp= -1;  getch(comptab);
  do { while ( ch==' ')  getch(comptab);
       rdhx (comptab); idtab[++idp ]= val; } while (idp!=70); */

    idtab[0]=0x8000;    idtab[1]=0xAB00;
    idtab[2]=0xAD00;    idtab[3]=0xCF52;
    idtab[4]=0x5052;    idtab[5]=0xCF43;
    idtab[6]=0x4655;    idtab[7]=0xCE43;
    idtab[8]=0xBD00;    idtab[9]=0xA300;
    idtab[10]=0xBC00;    idtab[11]=0xBE00;
    idtab[12]=0xAA00;    idtab[13]=0x4449;
    idtab[14]=0xD600;    idtab[15]=0x4D4F;
    idtab[16]=0xC400;    idtab[17]=0x414E;
    idtab[18]=0xC400;    idtab[19]=0x494E; 
    idtab[20]=0xC300;    idtab[21]=0x4445; 
    idtab[22]=0xC300;    idtab[23]=0x4152; 
    idtab[24]=0x5241;    idtab[25]=0xD900;
    idtab[26]=0x5641;    idtab[27]=0xD200;
    idtab[28]=0x4245;    idtab[29]=0x4749;
    idtab[30]=0xCE00;    idtab[31]=0xC946;
    idtab[32]=0x5748;    idtab[33]=0x494C;
    idtab[34]=0xC500;    idtab[35]=0x5245; 
    idtab[36]=0x5045;    idtab[37]=0xC154;
    idtab[38]=0x5245;    idtab[39]=0x5455;
    idtab[40]=0xD24E;    idtab[41]=0x4E4F;
    idtab[42]=0xD400;    idtab[43]=0x4745;
    idtab[44]=0xD400;    idtab[45]=0x5055;
    idtab[46]=0xD400;    idtab[47]=0x4841;
    idtab[48]=0xCC54;    idtab[49]=0xAC00;
    idtab[50]=0x5448;    idtab[51]=0xC54E;
    idtab[52]=0xC44F;    idtab[53]=0x454E;
    idtab[54]=0xC400;    idtab[55]=0x554E;
    idtab[56]=0x5449;    idtab[57]=0xCC00;
    idtab[58]=0x454C;    idtab[59]=0xD345;
    idtab[60]=0xBB00;    idtab[61]=0xA800;
    idtab[62]=0xA900;    idtab[63]=0xAE00;
    idtab[64]=0xDB00;    idtab[65]=0xDD00;
    idtab[66]=0xDF00;    idtab[67]=0xA700;
    idtab[68]=0xA200;    idtab[69]=0xA400;
    idtab[70]=0xA500;

  /* for (i=0; i<70; i++) pname(i); */
  /*  for (i=0; i<70; i++) {  x=idtab[i];
     fprintf(stderr, "%d %4x %c%c \n", i, x & 0xffff, (x >>8) & 0x7f, x & 0x7f); }  */

  idp=70; ch=' ';
  line= 0;
  outcnt= 0;  chcnt= 0; rcodecnt=0;
  nogl= 46; noid= 46; nolo= 46;
  psmode= 0;
  psval= 0;

  putch('*'); /* pushlc=0*/
  putch('('); /*JMP*/

  insymbol();
  /* fprintf(stderr, "sy=%d\n", sy);*/

  if (sy==18 )  {  insymbol(); declare(35, 1) ; };
  if (sy==19) {
    idps= noid-1;
    insymbol();
    declare(35, 0);
    while (++idps  !=  noid) pnhx4(0) ; };
  nogl= noid;
  idps= idp;
  while ( sy / 2==3  )  {  /* void | short int */
    funcp= (sy==7);
    insymbol();
    if ( sy )  error(27);
    putch('$');
    pname(idr );
    insymbol();
    check(36);
    if (sy==18) {
      insymbol() ;
      declare(35,0); };
    declare(37, 0);
    check(35);
    pnhx2(noid-nogl);
    if (sy==19 ) {
      insymbol(); declare(35,0) ; };
    pnhx2(noid-nogl); 
    statement();  /* proc | func body */
    if (funcp) { genps(2,0); gencode1(0xc1) ; }
    else gencode1(0xc0); 
    check(35);
    idp= idps; noid= nogl; nolo= nogl;} ;
  /* main program starts here */
  putch('!');
  putch('/');  /* embed a jump instruction at the beginning to jump here */
  statement(); 
  gencode1(0xc3) ;
  if (sy != 38) error(38);
  putch('.');
  putch('\n');

  /****************************************************************/
  fprintf(stderr, "source=%s  is compiled by mpc and %d rcode written to %s in %d us\n",
	sfilename, rcodecnt, rfilename, clock()); }


