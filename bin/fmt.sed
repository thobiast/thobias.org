#!/bin/sed -f
#
# script  para identar um texto. ele identa par�grafos quebrando as
# linhas  do  mesmo  em  65 caracteres e depois justifica as linhas
# (adicionando espa�os entre palavras).
#
# Thobias Salazar Trevisan
# 
# �ltima Atualiza��o: 16/04/2004
#
##################### Precisa de sed >= 4.0 #######################

# pega um par�grafo inteiro e coloca no hold space
/./{H;$!d;}

# recupera o par�grafo
x
# Apaga o \n do in�cio da linha e troca os outros por espa�o
s/^\n//; s/\n/ /g
# identa, ie, coloca um \n a cada 64 chars
s/\(.\{,64\}\b\)/\1\n/g
# se o sinal ficou em uma linha separada volta para mesma linha
s/\n\([])[:punct:]]*\)$/\1/

# agora come�a a justificar o texto
:ini
/^.\{,65\}/{
	# se n�o tem, coloca um \n no final. como � par�grafo, tem que ter
	# linha em branco entre eles.
	s/ *$//; s/\([^\n]\)$/\1\n/
	s/\n /\n/g
	# guarda uma c�pia do par�grafo no reserva
	h
	# pega somente a primeira linha
	s/\n.*//
	# arruma os espa�os
	s/^ \+//
	s/ \+/ /g
	s/ \+$//

		# aqui come�a o algoritmo para justificar
		s/ /�1�/g
		:a
		/�1�/{s//�2�/;bb;}
		/�2�/{s//�3�/;bb;}
		/�3�/{s//�4�/;bb;}
		s/.*//
		x;b;
			
		:b
		s/�4�/    /g
		s/�3�/   /g
		s/�2�/  /g
		s/�1�/ /g
		tcheck

		# teste se precisa adicionar mais espa�os
		:check
		/^.\{65\}/!{
			s/    /�4�/g
			s/   /�3�/g
			s/  /�2�/g
			s/ /�1�/g
			ba
		}
		# se chegou aqui � porque a linha j� est� justificada,
		# ent�o imprime
		p
		# volta o par�grafo original e se tiver \n
		# apaga a primeira linha (que j� foi justificada e
		# pula para o ini
		g;/\n/{s/[^\n]\+\n//;bini;}
		# se era a �ltima linha do par�grafo, limpa o hold space
		x;s/.*//;x
		# pula para o final do script
		b
}
p

