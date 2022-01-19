# Micro-Plan
## C版microplan コンパイラとローダ､実行系

マイクロプランコンパイラは、microplan 自身で記述されているが、そのままでは実行できないので、Cに書き換える。コンパイラの本体は、mpc.c として再現した｡8080のアセンブリで書かれたR-codeローダ、Q-codeインタプリタもCで記述する｡
マイクロプランのソースプログラムは､mpcでR-codeにコンパイルされ､拡張子=.rc のR-codeファイルにストアされる｡R-codeローダである mpld は、このR-code ファイルを読み込んで､拡張子=.qc のQ-codeファイルを作成する｡同時に､拡張子=.sym のシンボル表を書き出す｡シンボル表には、関数と手続きの名前とその16進アドレスが列挙される｡最後に､メインプログラムのエントリアドレスが追記される｡その名前は、mainと小文字であることに注意｡できあがった.qc, .sym ファイルを参照して､mpx がQ-codeを解釈実行する｡実行に.symファイルは不要であるが､-d オプションを与えると、実行トレースが表示され､CALL命令のオペランドとして手続き･関数名が表示できる｡さらに、mpqdisasm は、.qc と.symから、Q-code を逆アセンブルする｡

### mpc [-d] <filename>

<filename>.mp をマイクロプランのソースコードとしてコンパイルし、結果のR-codeを<filename>.rc に書き出す。-dを付けると、コンパイル中に読み込んだソースコードをstderrに順次表示するので、error が出た場合、その場所がわかる。

### mpld [-d] <filename>

  <filename>.rc をRコードのファイルとして入力し、ロードした結果のメモリイメージを<filename>.qc に書き出す。また、シンボル表を<filename>.sym に書き出す。

### mpx [-d] [-c] [-r] [-i <infile>] [-o <outfile>] <filename>

    <filename>.qc をQコードファイルとして読み込み、また<filename>.sym をシンボル表として読み込んで、Qコードの先頭(ゼロ番地）から実行を開始する。ゼロ番地には､メインプログラムへのジャンプ命令が入っている｡最後に、実行された命令数とclock数(マイクロ秒単位）が表示される｡
-i, -o は、実行されるマイクロプランプログラムの入出力ファイルを指定する｡指定が無い場合､stdin と stdout が適用される｡
-d (debug) を指定すると､実行される命令が逆アセンブル表示される　(トレースモード）。
-c は、call traceモードで、call命令が実行されると何が呼び出されるか表示

### mpq [-t <symfile>] <filename>
Q-code である <filename>.qc とそのシンボル表である<filename>.sym を読み込んで、逆アセンブルしたリストを表示(stdout)する｡
