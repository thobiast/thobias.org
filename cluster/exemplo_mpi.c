/*
 * Simples programa utilizando a biblioteca paralela de troca de 
 * mensagens MPI (Message-Passing Interface)
 *
 * Este eh um algoritmo simples de um contador paralelo. Utiliza a ideia de
 * um token, onde cada processo recebe um contador do seu processo a esquerda, 
 * soma 1 a este contador e o envia para o processo a sua direito.
 * OBS: Processo 0 nao recebe o contador do seu processo a esquerda, ele simplesmente
 * envia o contador mais 1 para o processo a sua direita
 *      O ultimo processo apenas recebe o contador do processo a sua esquerda e imprime o
 * contador final 
 *
 * Como executar:
 * Utilizando a implementação LAM
 *
 *    lamboot -v nodes.txt
 *    mpicc -o exemplo_mpi exemplo_mpi.c
 *    mpirun -np 3  ./exemplo_mpi
 *    wipe nodes.txt
 *
 * OBS: onde nodes.txt eh um arquivo contendo os nos que irao participar.
 *
 *
 * Data:  Fri Sep 27 BRT 2002
 * Desenvolvido por Thobias Salazar Trevisan <thobias at lcp.coppe.ufrj.br>
 */
 
#include "mpi.h"  // Biblioteca necessaria para o MPI
#include <stdio.h>
#include <stdlib.h>

#define WORKTAG 1

int main( argc, argv )
int argc;
char **argv;
{
        int rank, size, *contador;
        MPI_Status  status;

	//printf("Inicializando MPI...\n");
        MPI_Init( &argc, &argv );
	// Coloca em rank, qual o contador do meu processo
        MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
	// Coloca em size o total de processos
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        if (rank ==0) printf("\nTotal de processos = %d\n\n\n",size);

	contador = (int *) malloc(sizeof(int));
	if (!contador){
		printf("Erro malloc contador, meu rank = %d\n",rank);
		MPI_Finalize();
		exit(0);
	}

	*contador=0;
	
        // Barreira, para sincronizar a execucao todos os processos neste ponto	
	// excao so continua quando todos os processos chegarem nesta barreira
	MPI_Barrier(MPI_COMM_WORLD);
	// Se for processo 0, soma 1 em contador e envia msg para o proximo processo
	if (rank == 0) {
		(*contador)++;
		printf("meu rank = %d, contador = %d, enviar msg para rank = %d\n",rank,*contador,rank+1);
 	 	MPI_Send(contador, 1, MPI_INT, rank+1, WORKTAG, MPI_COMM_WORLD);
	}
	else {
		// Se nao somos processos 0, recebemos msg do processo anterior
		MPI_Recv(contador,1,MPI_INT, rank-1,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	    	// Se nao somos o ultimo processo enviamos msg para o proximo
	    	if (rank != (size-1)) {
			printf("meu rank = %d, recv msg rank = %d, contador = %d, enviar msg rank = %d\n",rank,rank-1,*contador,rank+1);
			(*contador)++;
 	    		MPI_Send(contador, 1, MPI_INT, rank+1, WORKTAG, MPI_COMM_WORLD);
	    	}
	    	else {
			(*contador)++;
	     		printf("meu rank = %d, recv msg rank = %d, contador = %d\n",rank,rank-1,*contador);
			printf("\nvalor final de contador = %d\n",*contador);
	    	}
	}
	// Barreira para sincronizar a execucao de todos os processos
	MPI_Barrier(MPI_COMM_WORLD);
	// Finaliza o MPI
	MPI_Finalize();
}
									 
