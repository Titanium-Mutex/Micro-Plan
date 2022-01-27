%  MicroPlan Compiler:  %
%  %
%  This program is published in the article [1].  %
%   [1] S. Tomura: "MicroPlan Compiler", Microcomputer Programming,  %
%        bit Feb.extra issue,  Vol.10, No. 3, pp.112-144,  %
%        Kyoritsu Shuppan Co. Ltd., 1978. (in Japanese)   %
%  %
%  Copyright 1978 Satoru Tomura  %
%  %
%  This program is free software: you can redistribute it and/or modify  %
%  it under the terms of the GNU General Public License as published by  %
%  the Free Software Foundation, either version 3 of the License, or  %
%  any later version.  %
%  %
%  This program is distributed in the hope that it will be useful,  %
%  but WITHOUT ANY WARRANTY; without even the implied warranty of  %
%  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  %
%  GNU General Public License for more details.  %
%  %
%  You should have received a copy of the GNU General Public License  %
%  along with this program.  If not, see <https://www.gnu.org/licenses/>.  %

ARRAY IDTAB[255];
VAR      CH,SY,VAL,PSMODE,PSVAL, IDP, IDQ, IDR, IDN, NOGL, NOLO,
         NOID, IDPS, FUNCP, LINE, CHCNT, OUTCNT;

PROC PUTC(CH);
   IF CH THEN BEGIN
      PUT(CH);
      IF INC(OUTCNT)=80 THEN BEGIN
         PUT($0A);
         OUTCNT_0   END;
      END;

PROC PNHX1(N);
   IF N<10 THEN PUTC(N+48) ELSE PUTC(N+55);

PROC PNHX2(N);
   BEGIN PNHX1(N DIV 16); PNHX1(N AND $F) END;

PROC PNHX4(N);
   BEGIN
      PNHX2(N DIV 256 AND  $FF);
      PNHX2(N AND $FF) END;
      
PROC CLEARPS();
   IF  PSMODE THEN  BEGIN
      IF PSMODE=3 THEN BEGIN 
         PNHX2(3); PNHX4(PSVAL)
         END
      ELSE BEGIN
         PNHX2(PSMODE);
         PNHX2(PSVAL) END;
      PSMODE_0
      END;
      

PROC GENEX(OP);
   IF PSMODE THEN BEGIN
      IF PSMODE=3 THEN BEGIN
        PNHX2(OP+3);
        PNHX4(PSVAL)  END
      ELSE BEGIN
        PNHX2(OP+PSMODE); PNHX2(PSVAL)
        END; 
      PSMODE_0 
      END
   ELSE PNHX2(OP);

PROC GENCODE1(OP);
   BEGIN
      CLEARPS(); PNHX2(OP) END;
     
PROC GENCODE2(OP,VAL);
   BEGIN
      CLEARPS(); PNHX2(OP); PNHX2(VAL);
      END;
      
PROC GENPS(MODE,VAL);
   BEGIN
      CLEARPS();
      PSMODE_MODE; PSVAL_VAL
      END;

PROC ERROR(N); 
   BEGIN
      PUT($0A);
      OUTCNT_0; 
      PUT('ERR ');
      PUT(N+42);
      PUT(" ); 
      PNHX4(LINE);
      PUT(" ); 
      PNHX2(CHCNT);
      PUT(" );
      PNHX2(SY);
      PUT(" );
      PNHX4(VAL);
      PUT("  , CH, $0A);
      HALT();
      END;

FUNC  ISLETTER();
   RETURN("@ < CH) AND (CH < "[ );

FUNC ISDIGIT();
   RETURN  ("/ < CH) AND (CH< ": );
   
PROC GETCH();
   BEGIN
      CHCNT_INC(CHCNT);
      REPEAT
         CH_GET() AND $7F;
         IF CH=$0D THEN BEGIN
            LINE_INC(LINE);
            CHCNT_0;
            CH_"  
            END 
         UNTIL ($1F<CH) AND (CH<$60)
      END;

PROC  RDHX();
   BEGIN
      VAL_0;
      WHILE(ISLETTER()  OR  ISDIGIT()) AND(CH<"G)  DO 
         BEGIN
            IF   CH>"@  THEN  VAL_VAL*16+CH-55
            ELSE   VAL_VAL*16+CH-"0;
            GETCH() END
      END;
 
PROC INSYMBOL();
   VAR  I,  J, L;
   BEGIN
      REPEAT
         WHILE CH="  DO GETCH();
         IF ISDIGIT() THEN BEGIN
            SY_2;
            VAL_0;
            REPEAT
               VAL_VAL*10+CH-"0;
               GETCH()
               UNTIL NOT ISDIGIT()
            END
         ELSE BEGIN
            L_1;
            IDQ_IDP;
            IF ISLETTER() THEN
               BEGIN
                  REPEAT
                    IF INC(L) AND 1 THEN IDTAB[IDQ]_IDTAB[IDQ]+CH
                    ELSE   IF   IDQ<254
                       THEN IDTAB[INC(IDQ)]_CH*256 
                       ELSE ERROR(23);
                    GETCH()
                    UNTIL NOT(ISLETTER() OR ISDIGIT());
                 IDTAB[IDQ]_IDTAB[IDQ]+$8000;
                 L_L DIV 2 END
            ELSE BEGIN
               IDTAB[INC(IDQ)]_CH*256+$8000;
               GETCH()
               END;
            IDR_IDP;
            IDN_NOID;
            REPEAT
               REPEAT
                  J_IDR;
                  REPEAT UNTIL IDTAB[DEC(IDR)]<0;
                  UNTIL (J-IDR=L) OR (DEC(IDN)=2);
               I_IDP;
               J_IDR;
               REPEAT
                  UNTIL (IDTAB[INC(I)] # IDTAB[INC(J)]) OR (I>IDQ)
               UNTIL (I> IDQ)  OR  (IDN=2) ;
           IF  IDN=2 THEN BEGIN
              SY_0;
              IDR_IDP END
         ELSE IF IDN<43 THEN SY_IDN
         ELSE IF IDN=43 THEN BEGIN
           SY_2; VAL_CH;
           GETCH() END
         ELSE  IF  IDN=44  THEN  BEGIN SY_2; RDHX() END
         ELSE IF IDN=45 THEN BEGIN 
            WHILE CH#"% DO GETCH();
            GETCH();
            SY_45 END
         ELSE BEGIN
            SY_1;
            IF  IDN<NOGL THEN VAL_IDN-46
            ELSE IF IDN<NOLO THEN VAL_IDN-NOGL+ $81
            ELSE SY_0
            END
         END
     UNTIL SY#45;
%   PUT('INSYMBOL: SY=', SY DIV 10 + "0, SY MOD 10 +"0, $0A); %
   END;
     
PROC CHECK(S);
   IF SY=S THEN INSYMBOL() ELSE ERROR(S);

FUNC BUT(S); 
  IF SY=S THEN BEGIN INSYMBOL(); RETURN 0 END ELSE RETURN 1;

PROC PNAME(R);  % print string in the idtab designated by R %
VAR X;
BEGIN
   REPEAT
      X_ IDTAB [INC(R)];
      PUTC((X AND $7F00)  DIV $100);
      PUTC(X AND $7F)
      UNTIL  X<0;
   PUTC(" ) END;
 
PROC FACTOR();
   VAR SYP, VALP;
   BEGIN
   % PUT('FACTOR',$0A); %
   IF SY THEN BEGIN
      SYP_SY;
      VALP_VAL;
      INSYMBOL();
      IF  SYP=2  THEN
         IF VALP AND $FF00 THEN GENPS(3,VALP) ELSE GENPS(2,VALP)
      ELSE IF SYP=36 THEN BEGIN
         EXPRESSION();
         CHECK(37) END
      ELSE IF SYP DIV 2=8 THEN BEGIN 
         CHECK(36);
         IF SY#1 THEN ERROR(26); GENCODE2(SYP+113, VAL); INSYMBOL();
            CHECK(37) END
      ELSE IF SYP=25 THEN BEGIN FACTOR();
         GENCODE1($C5) END
      ELSE IF SYP=26 THEN BEGIN CHECK(36);
         CHECK(37); GENCODE1($C2) END
      ELSE  IF  SYP=1  THEN
         IF SY#39  THEN  GENPS(1, VALP)
         ELSE BEGIN
            SUBSCRIPT(); GENCODE2($05,VALP) END
      ELSE ERROR(29) END
   ELSE PCALL() ;
   END;
   
PROC TERM();
VAR  OP;
   BEGIN
      % PUT('TERM',$0A); %
      FACTOR();
      WHILE  SY  DIV  4=3 DO BEGIN % mutiplying operator %
         OP_4*SY-8;
         INSYMBOL();
         FACTOR();
         GENEX(OP)
         END END;
         
PROC   SIMPLE();
   VAR OP;
   BEGIN
      % PUT('SIMPLE',$0A); %
      IF SY DIV 3=1 THEN GENPS(2,0) ELSE TERM();
      WHILE  SY  DIV  3=1    DO  BEGIN
         %<adding operator>%
         OP_4*SY+16; INSYMBOL(); TERM(); GENEX(OP) END END;
         
PROC  EXPRESSION();
   VAR OP;
   BEGIN
      % PUT('EXPRESSION',$0A); %
      SIMPLE();
      IF  SY DIV 4=2  THEN  BEGIN  %<relational operator>%
         OP_4*SY-20;
         INSYMBOL();
         SIMPLE();
         GENEX(OP)  END
      END;
      
PROC SUBSCRIPT();  % array subscript %
   BEGIN INSYMBOL(); EXPRESSION(); CHECK(40) END;

PROC PCALL() ; % proc/func call %
   VAR P, R, N; 
   BEGIN
      % PUT('PCALL',$0A); %
      P_IDP; R_IDR; N_NOID;
      IF IDP=IDR THEN BEGIN IDP_IDQ; NOID_INC(NOID); END;
      INSYMBOL();
      CHECK(36);
      %If the name is not registered in IDTAB, input there temporarily for output%
      IF SY#37 THEN REPEAT EXPRESSION() UNTIL BUT(29); 
      CHECK(37);
      CLEARPS();
      PUTC("#); %call instruction %
      PNAME(R);
      NOID_N; IDP_P
      END;

PROC STATEMENT(); 
   VAR SYP,VALP;
   IF SY  DIV  4#8  THEN %not END, UNTIL, ELSE, or ';' %
      IF SY THEN BEGIN  %defined id; if sy==0, it is a func/proc call%
         SYP_SY; VALP_VAL;
         % PUT('STATEMENT', $0A); %
         INSYMBOL();
         IF  SYP=1  THEN  % variable id %
            IF  SY=41  THEN  BEGIN  %becomes  --> assignement statement%
               INSYMBOL(); EXPRESSION(); GENCODE2($80,VALP) %PULL% END
            ELSE IF SY=39 THEN BEGIN  %left bracket [%
               SUBSCRIPT(); CHECK(41); %becomes%
               EXPRESSION();
               GENCODE2($09,VALP) END  %PULL MODIFIED%
            ELSE  ERROR(28)  %illegal statement%
         ELSE IF SYP=20 THEN BEGIN %begin%
            REPEAT STATEMENT() UNTIL BUT(35); %semicolon ;%
            CHECK(32) %END %
            END
         ELSE IF SYP=21 THEN BEGIN EXPRESSION(); %IF%
            CLEARPS(); PUTC("*);
            PUTC("));
            CHECK(30); STATEMENT();
            IF SY=34 %else% THEN BEGIN PUTC("*);
               PUTC("();  PUTC("%); PUTC("/); INSYMBOL();
               STATEMENT() END;
            PUTC("/) END
         ELSE IF SYP=22 THEN BEGIN PUTC("*); %while%
            EXPRESSION(); CLEARPS(); CHECK(31); PUTC("*);
            PUTC(")); STATEMENT(); PUTC("%);
            PUTC("+); PUTC("/) END
         ELSE IF SYP=23 THEN BEGIN PUTC("*);
            REPEAT STATEMENT() UNTIL BUT(35); CHECK(33);
            EXPRESSION(); CLEARPS(); PUTC("-); END
         ELSE IF SYP=24 THEN BEGIN EXPRESSION() ; %return%
            GENCODE1($C1); END
         ELSE IF SYP=27 THEN BEGIN %put%
            CHECK(36); %left paren%
            REPEAT
               IF SY=42 THEN BEGIN GENCODE1($C6);
                  WHILE CH#"' DO BEGIN
                     IF  CH=""  THEN GETCH();  %escape%
                     PNHX2(CH); GETCH() END;
                  PNHX2(0); GETCH(); INSYMBOL() END
               ELSE BEGIN EXPRESSION() ; GENCODE1($C4) END
               UNTIL BUT(29);
            CHECK(37) END %rparen%
         ELSE IF SYP=28 THEN BEGIN CHECK(36); %halt()%
            CHECK(37); GENCODE1($C3) END
         ELSE ERROR(28)
         END %if sy%
      ELSE PCALL();

PROC DECLARE(S,B); %s: stop symbol, b:flag for global array decl%
  BEGIN
    IF SY#S THEN REPEAT
      IF  (SY<2)AND (IDN<NOGL) THEN  BEGIN
        IF  (NOID-NOGL) > 126  THEN ERROR(24);
        NOLO_INC(NOID);
        IDP_IDQ; %register in the idtab$B(J%
        INSYMBOL();
        IF  B  THEN  BEGIN
          CHECK(39);
          IF SY#2 THEN ERROR(25);
          PUTC("");
          PNHX4(INC(VAL) *2); INSYMBOL(); CHECK(40)
          END END
      ELSE ERROR(27)
    UNTIL BUT(29);
  CHECK(S) END;

% main program %
BEGIN
    IDTAB[0]_ $8000;    IDTAB[1]_ $AB00;
    IDTAB[2]_ $AD00;    IDTAB[3]_ $CF52;
    IDTAB[4]_ $5052;    IDTAB[5]_ $CF43;
    IDTAB[6]_ $4655;    IDTAB[7]_ $CE43;
    IDTAB[8]_ $BD00;    IDTAB[9]_ $A300;
    IDTAB[10]_ $BC00;    IDTAB[11]_ $BE00;
    IDTAB[12]_ $AA00;    IDTAB[13]_ $4449;
    IDTAB[14]_ $D600;    IDTAB[15]_ $4D4F;
    IDTAB[16]_ $C400;    IDTAB[17]_ $414E;
    IDTAB[18]_ $C400;    IDTAB[19]_ $494E; 
    IDTAB[20]_ $C300;    IDTAB[21]_ $4445; 
    IDTAB[22]_ $C300;    IDTAB[23]_ $4152; 
    IDTAB[24]_ $5241;    IDTAB[25]_ $D900;
    IDTAB[26]_ $5641;    IDTAB[27]_ $D200;
    IDTAB[28]_ $4245;    IDTAB[29]_ $4749;
    IDTAB[30]_ $CE00;    IDTAB[31]_ $C946;
    IDTAB[32]_ $5748;    IDTAB[33]_ $494C;
    IDTAB[34]_ $C500;    IDTAB[35]_ $5245; 
    IDTAB[36]_ $5045;    IDTAB[37]_ $C154;
    IDTAB[38]_ $5245;    IDTAB[39]_ $5455;
    IDTAB[40]_ $D24E;    IDTAB[41]_ $4E4F;
    IDTAB[42]_ $D400;    IDTAB[43]_ $4745;
    IDTAB[44]_ $D400;    IDTAB[45]_ $5055;
    IDTAB[46]_ $D400;    IDTAB[47]_ $4841;
    IDTAB[48]_ $CC54;    IDTAB[49]_ $AC00;
    IDTAB[50]_ $5448;    IDTAB[51]_ $C54E;
    IDTAB[52]_ $C44F;    IDTAB[53]_ $454E;
    IDTAB[54]_ $C400;    IDTAB[55]_ $554E;
    IDTAB[56]_ $5449;    IDTAB[57]_ $CC00;
    IDTAB[58]_ $454C;    IDTAB[59]_ $D345;
    IDTAB[60]_ $BB00;    IDTAB[61]_ $A800;
    IDTAB[62]_ $A900;    IDTAB[63]_ $AE00;
    IDTAB[64]_ $DB00;    IDTAB[65]_ $DD00;
    IDTAB[66]_ $DF00;    IDTAB[67]_ $A700;
    IDTAB[68]_ $A200;    IDTAB[69]_ $A400;
    IDTAB[70]_ $A500;

%   IDP_-1; %
%   REPEAT PNAME(INC(IDP)) UNTIL IDP=69;  %
%  REPEAT  WHILE  CH="  DO  GETCH(); RDHX(); IDTAB[INC(IDP)]_VAL UNTIL IDP=70; %

  IDP_70; CH_" ;
  LINE_0;
  OUTCNT_0;
  CHCNT_0;
  NOGL_46; NOID_46; NOLO_46;
  PSMODE_0;
  PSVAL_0;

  % two bytes for a jump instruction before global variables %
  PUTC("*); %pushlc=0%
  PUTC("(); %JMP%
  INSYMBOL();
  IF SY=18  THEN  BEGIN  INSYMBOL(); DECLARE(35, 1) END; %ARRAY%
  IF SY=19 THEN BEGIN  %VAR%
    IDPS_NOID-1;
    INSYMBOL();
    DECLARE(35, 0);
    WHILE INC(IDPS)# NOID DO PNHX4(0) END;
  NOGL_NOID;
  IDPS_IDP;
  WHILE  SY  DIV  2=3   DO  BEGIN  % proc or func %
    FUNCP_SY=7;
    INSYMBOL();
    IF  SY  THEN  ERROR(27);
    PUTC("$);
    PNAME(IDR );
    INSYMBOL();
    CHECK(36);
    IF SY=18 THEN BEGIN   %local array%
       INSYMBOL() ;
       DECLARE(35,0)
       END;
    DECLARE(37, 0);
    CHECK(35);
    PNHX2(NOID-NOGL);
    IF SY=19  THEN BEGIN  %loca var%
       INSYMBOL(); DECLARE(35,0) END;
    PNHX2(NOID-NOGL); 
    STATEMENT();  % proc or func body %
    IF FUNCP THEN BEGIN GENPS(2,0); GENCODE1($C1) END
    ELSE GENCODE1($C0); 
    CHECK(35);
    IDP_IDPS; NOID_NOGL; NOLO_NOGL;
    END;
  PUTC("!);
  PUTC("/);  % embed a jump instruction at the beginning to jump here %
  STATEMENT(); 
  GENCODE1($C3) ;
  IF SY#38 THEN ERROR(38);
  PUTC(".);
  PUT($0D,$0A)
  END. 



