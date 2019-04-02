#!/bin/sed -nf
#
# Thobias Salazar Trevisan <thobias at lcp.coppe.ufrj.br> 
#
# Mini 'indentador' de shell script. Deixa do, then, else, done, fi
# sozinhos em uma linha.
#
# Note que para estas palavras-chave estejam na mesma linha
# � necess�rio uma ;. Ex: if [ "$x" ];then
#
# Ent�o temos que substituir este ponto-e-v�rgula por \n (new line)
#
# assim, o problema a resolver �:
# trocar ; por \n quanto necess�rio, ou seja, n�o estar entre par�nteses, 
# crases, aspas, aspas-duplas, chaves, colchetes e n�o for coment�rio.
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
	:a;s/^\([^"]*\)"\([^"]*\);\([^"]*\)"/\1�1\2�\3�1/;ta
	:b;s/^\([^']*\)'\([^']*\);\([^']*\)'/\1�2\2�\3�2/;tb
	:c;s/^\([^`]*\)`\([^`]*\);\([^`]*\)`/\1�3\2�\3�3/;tc
	:d;s/^\([^(]*\)(\([^)]*\);\([^)]*\))/\1�4\2�\3�5/;td
	:e;s/^\([^{]*\){\([^}]*\);\([^}]*\)}/\1�6\2�\3�7/;te
	:f;s/^\([^[]*\)\[\([^]]*\);\([^]]*\)\]/\1�8\2�\3�9/;tf
	s/; *\(do\|then\|else\|done\|fi\)/\
\1/g
	s/�/;/g;
	s/�1/"/g;s/�2/'/g;s/�3/`/g;s/�4/(/g;s/�5/)/g
	s/�6/{/g;s/�7/}/g;s/�8/[/g;s/�9/]/g
	s/\(do\|then\|else\|done\|fi\) *;\?\( *[[:alnum:]]\+\)/\1\
 \2/g
	p;
	x;s/[^\n]\+\n\(.*\)/\1/p
	b
}
:sai;p


