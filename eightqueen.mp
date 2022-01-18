% eight queens %

ARRAY X[7], COL[7], UP[14], DOWN[14];
VAR N, K;

PROC GENERATE();
VAR H;
BEGIN
   H_0;
   REPEAT
      % PUT('N=', N+"0, ' H=', H+"0, ' COL[H]=',COL[H]+"0,
	 ' UP=', UP[N-H+7]+"0, ' DOWN=', DOWN[N+H]+"0, $0A); %
      IF COL[H] AND UP[N-H+7] AND DOWN[N+H] THEN BEGIN
         X[N]_H;
         COL[H]_0;
         UP[N-H+7]_0;
         DOWN[N+H]_0;
	 % PUT('N=',"0+N, $0A); %
         IF INC(N)=8 THEN BEGIN
            K_0;
            REPEAT PUT(' ', X[K]+"0) UNTIL INC(K)=8;
            PUT($0A) END
         ELSE GENERATE();
      N_DEC(N);
      DOWN[N+H]_1;
      UP[N-H+7]_1;
      COL[H]_1;
      END
    UNTIL INC(H)=8 END; 

BEGIN
   N_0;
   K_0; REPEAT COL[K]_1 UNTIL INC(K)=8;
   K_0; REPEAT UP[K]_1; DOWN[K]_1; UNTIL INC(K)=15;
   PUT($0A, '*** EIGHT QUEENS ***', $0A);
   GENERATE(); END.
 
