#!/bin/sed -f
#
# script para transformar código C em código colorido em HTML.
# o código tem que ficar entre <pre> </pre> que são adicionados
# pelo script. caso contrário tem que adicionar <br> no final das 
# linhas, '&nbsp;' no início....
#
# OBS:
# Este script tem alguns "bugs", ele foi feito em 20min só para
# atender minhas necessidades na escrita de um tutorial
#
# OBS2: Precisa de sed >= 4.0
#
# Última atualização: 16-06-2004
#
# Thobias Salazar Trevisan 
#

s/&/\&amp;/g
s/</\&lt;/g
s/>/\&gt;/g
s/"/\&quot;/g
s/'/\&#039;/g

# colore mensagens, ie, strings entre "
s/\([^=]\)\(\&quot;.*\&quot;\)/\1<font color="#FF0066">\2<\/font>/g

# colorize os includes
/^#include/s/^#include \(.*\)/<font color="purple">#include<\/font> <font color="Orchid">\1<\/font>/

# colorize os comentários
# OBS: pega somente comentários do tipo /**/ e que estão sozinho na(s)
# linha(s)
\,^\([ \t]\)*\/\*,{
1s/^/<pre>\n/
:a
/\*\//!{N;ba;}
s/\(<pre>\n\)\?\(.*\)/\1<font color="blue">\2<\/font>/
b
}

# colore palavras reservadas
s/\b\(if\|else\|while\|return\)\b/<font color="#FF0000">\1<\/font>/g
s/\b\(int\|char\|void\|size_t\)\b/<font color="#339900">\1<\/font>/g
s/\b\(stderr\|stdout\|NULL\)\b/<font color="#FF0066">\1<\/font>/g
# colore numeros sozinhos 
s/\([0-9]\)\+\([^0-9"]\)/<font color="#FF0066">\1<\/font>\2/g

1s/^/<pre>\n/
$s/$/\n<\/pre>/ 
