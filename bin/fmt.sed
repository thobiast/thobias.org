#!/bin/sed -f
#
# script  para identar um texto. ele identa parágrafos quebrando as
# linhas  do  mesmo  em  65 caracteres e depois justifica as linhas
# (adicionando espaços entre palavras).
#
# Thobias Salazar Trevisan
# 
# Última Atualização: 16/04/2004
#
##################### Precisa de sed >= 4.0 #######################

# pega um parágrafo inteiro e coloca no hold space
/./{H;$!d;}

# recupera o parágrafo
x
# Apaga o \n do início da linha e troca os outros por espaço
s/^\n//; s/\n/ /g
# identa, ie, coloca um \n a cada 64 chars
s/\(.\{,64\}\b\)/\1\n/g
# se o sinal ficou em uma linha separada volta para mesma linha
s/\n\([])[:punct:]]*\)$/\1/

# agora começa a justificar o texto
:ini
/^.\{,65\}/{
	# se não tem, coloca um \n no final. como é parágrafo, tem que ter
	# linha em branco entre eles.
	s/ *$//; s/\([^\n]\)$/\1\n/
	s/\n /\n/g
	# guarda uma cópia do parágrafo no reserva
	h
	# pega somente a primeira linha
	s/\n.*//
	# arruma os espaços
	s/^ \+//
	s/ \+/ /g
	s/ \+$//

		# aqui começa o algoritmo para justificar
		s/ /§1§/g
		:a
		/§1§/{s//§2§/;bb;}
		/§2§/{s//§3§/;bb;}
		/§3§/{s//§4§/;bb;}
		s/.*//
		x;b;
			
		:b
		s/§4§/    /g
		s/§3§/   /g
		s/§2§/  /g
		s/§1§/ /g
		tcheck

		# teste se precisa adicionar mais espaços
		:check
		/^.\{65\}/!{
			s/    /§4§/g
			s/   /§3§/g
			s/  /§2§/g
			s/ /§1§/g
			ba
		}
		# se chegou aqui é porque a linha já está justificada,
		# então imprime
		p
		# volta o parágrafo original e se tiver \n
		# apaga a primeira linha (que já foi justificada e
		# pula para o ini
		g;/\n/{s/[^\n]\+\n//;bini;}
		# se era a última linha do parágrafo, limpa o hold space
		x;s/.*//;x
		# pula para o final do script
		b
}
p

