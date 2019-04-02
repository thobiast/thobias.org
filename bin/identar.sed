#!/bin/sed -nf
#
# Thobias Salazar Trevisan <thobias at lcp.coppe.ufrj.br> 
#
# Mini 'indentador' de shell script. Deixa do, then, else, done, fi
# sozinhos em uma linha.
#
# Note que para estas palavras-chave estejam na mesma linha
# é necessário uma ;. Ex: if [ "$x" ];then
#
# Então temos que substituir este ponto-e-vírgula por \n (new line)
#
# assim, o problema a resolver é:
# trocar ; por \n quanto necessário, ou seja, não estar entre parênteses, 
# crases, aspas, aspas-duplas, chaves, colchetes e não for comentário.
#
# Testanto:
#
# prompt> echo 'if [ "X;" ; -a ";X" ]; then echo x ;else echo y; fi' | ./indentar.sed
# if [ "X;" ; -a ";X" ]
# then
#  echo x 
# else
#  echo y
# fi
#
# prompt> echo 'while [ test x; ];do #echo ";"' | ./indentar.sed
# while [ test x; ]
# do 
# #echo ";"
#
# prompt> echo 'while true;do while true; do' | ./indentar.sed
# while true
# do
#  while true
# do
#
# prompt> echo 'echo while true;do; echo x' | ./indentar.sed
# echo while true
# do
#   echo x
#
# pompt> echo ' #while true;do echo nada' | ./indentar.sed
# #while true;do echo nada
#
#
# Sex Mai  9 17:33:41 BRT 2003
# 
#

/do\|then\|else\|done\|fi/{
	/^ *#/bsai
	s/^\([^'"]*\)\(#.*\)/\1\
\2/
	h;s/\([^\n]*\).*/\1/
	:a;s/^\([^"]*\)"\([^"]*\);\([^"]*\)"/\1ø1\2¿\3ø1/;ta
	:b;s/^\([^']*\)'\([^']*\);\([^']*\)'/\1ø2\2¿\3ø2/;tb
	:c;s/^\([^`]*\)`\([^`]*\);\([^`]*\)`/\1ø3\2¿\3ø3/;tc
	:d;s/^\([^(]*\)(\([^)]*\);\([^)]*\))/\1ø4\2¿\3ø5/;td
	:e;s/^\([^{]*\){\([^}]*\);\([^}]*\)}/\1ø6\2¿\3ø7/;te
	:f;s/^\([^[]*\)\[\([^]]*\);\([^]]*\)\]/\1ø8\2¿\3ø9/;tf
	s/; *\(do\|then\|else\|done\|fi\)/\
\1/g
	s/¿/;/g;
	s/ø1/"/g;s/ø2/'/g;s/ø3/`/g;s/ø4/(/g;s/ø5/)/g
	s/ø6/{/g;s/ø7/}/g;s/ø8/[/g;s/ø9/]/g
	s/\(do\|then\|else\|done\|fi\) *;\?\( *[[:alnum:]]\+\)/\1\
 \2/g
	p;
	x;s/[^\n]\+\n\(.*\)/\1/p
	b
}
:sai;p


