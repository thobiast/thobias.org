/*
 * Simples programa utilizando o software de memoria compartilhada distribuida
 * (DSM - Distributed Shared Memory) TreadMarks
 *
 * Este eh um algoritmo simples de um contador paralelo, onde cada processo
 * soma 1 a variavel compartilhada. No final o processo 0 imprime o valor
 * final contido na variavel compartilhada.
 * 
 * Como executar:
 * gcc -o exemplo_tmk exemplo_tmk.c -I{DIR}/Tmk-1.0.3.2/include/ 
 * -L{DIR}/Tmk-1.0.3.2/lib.i386_linux2/ -lTmk_udp -lm
 *
 * Crie no seu home um arquivo chamado .Tmkrc contendo os nós que irao participar da
 * computação
 * 
 * ./exemplo_tmk
 * ou para mostrar o display dos processos rodando em outros nos
 * ./exemplo_tmk -- -x
 *
 *
 * Data:  Sun Sep 29 BRT 2002
 * Desenvolvido por: 
 * 		Thobias Salazar Trevisan <thobias at lcp.coppe.ufrj.br>
 */

#include <stdio.h>
#include <Tmk.h> // Biblioteca necessaria para o TreadMarks

int *contador_compartilhado;

main(argc, argv)
     int    argc;
     char **argv;
{
	int c;
	
	while ((c = getopt(argc, argv, "")) != -1);
	// Inicializamos o Tmk
	Tmk_startup(argc, argv);
	if (Tmk_nprocs <= 1) {
		printf("\nNumero de nos deve ser maior do que 1\n\n"); 
		//Tmk_exit(0);
	}

	// Se sou processo 0 aloco a area compartilhada
	if (Tmk_proc_id == 0) {
		printf("\nTotal de processos = %d\n",Tmk_nprocs);
		contador_compartilhado = (int *) Tmk_malloc(sizeof(int));
		if (!contador_compartilhado) {
			printf("Erro Tmk_malloc\n");
			Tmk_exit(-1);
	  	}
	  	printf("\nContador compartilhado alocado\n");
		// Inicializa variavel compartilhada em todos os processos
		Tmk_distribute((char *)&contador_compartilhado, sizeof(contador_compartilhado));
	  	printf("Tmk_distribute ok\n\n");
	}

	// Sincronizamos todos os processos
      	Tmk_barrier(0);
	
	Tmk_lock_acquire(1);
	printf("Processo = %d, contador = %d, vou somar 1 ao contador\n",Tmk_proc_id,*contador_compartilhado);
	(*contador_compartilhado)++;
	Tmk_lock_release(1);
	
      	Tmk_barrier(0);

	if (Tmk_proc_id == 0) 
		printf("\nValor final em contador_compartilhado = %d\n",*contador_compartilhado);

	// Finaliza o TreadMarks
	Tmk_exit(0);
}

