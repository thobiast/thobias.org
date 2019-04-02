/******************************************************************************
*       Arquitetura de Computadores COS703
*               3 Periodo de 2000
*       Prof. Eliseu M Chaves Filho
*       Nomes:  Thobias Salazar Trevisan
*               Leonardo B. Pinho
*               Rodrigo W. Fonseca
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//Deficicoes de codigos referentes a intrucoes, seus conjuntos e tipos

#define MIF_RRR  0
#define MIF_RRI  1
#define MIF_BRC  2
#define MIF_CALL 3

#define JMPL  0
#define ARITH 1
#define LOGIC 2
#define SET   3
#define MEM   5
#define RET   15

#define ADD   0
#define ADDU  1
#define SUB   2
#define SUBU  3
#define MUL   4
#define MULU  5

#define AND   0
#define OR    1
#define XOR   2
#define NAND  3
#define NOR   4
#define XNOR  5
#define NOT   6
#define LSL   7
#define LSR   8
#define ASR   9

#define SEQ   0
#define SNE   1
#define SLT   2
#define SLTU  3
#define SLE   4
#define SLEU  5
#define SGT   6
#define SGTU  7
#define SGE   8
#define SGEU  9

#define LDW   0
#define STW   4

#define ADDI  0
#define ADDUI 1
#define SUBI  2
#define SUBUI 3
#define ANDI  4
#define ORI   5
#define XORI  6
#define NANDI 7
#define NORI  8
#define XNORI 9
#define LSLI  10
#define LSRI  11
#define ASRI  12
#define LUI   13
#define JMP   15

#define BEQ   0
#define BNE   1
#define BLT   2
#define BLTU  3
#define BLE   4
#define BLEU  5
#define BGT   6
#define BGTU  7
#define BGE   8
#define BGEU  9
#define BRT   10
#define BRF   11

#define MAX   131072   //65535

// Declaracao da vetor de memoria e conjunto de registradores (32 bits)
// Enderecamento a palavra
unsigned long int memoria[MAX];
unsigned long int reg[32];

// Declaracao do contador de programa
unsigned long int pc;
float cpi;

// Acumulador de no. de intrucoes
unsigned long int num_inst,
	     clock,
	     total_inst;

// Registrador de pipeline existente entre fetch e decode
struct fi{
   int flag;
   unsigned long int pc;
   unsigned long int inst;
} fi;

// Registrador de pipeline existente entre decode e execute
struct di{
   int flag;
   unsigned long int op1;
   unsigned long int op2;
   unsigned long int op3;
   unsigned long int rs1;
   unsigned long int rs2;
   unsigned long int rd;
   unsigned long int uimm;
   unsigned long int simm;
   unsigned long int pc;
} di;

// Registrador de pipeline existente entre execute e result
struct ex{
   int flag;
   unsigned long int res;
   unsigned long int rd;
} ex;

// Mecanismo de forwarding
struct forwarding{
   unsigned long int id1;
   unsigned long int id2;
   unsigned long int data1;
   unsigned long int data2;
} fw;

//Variaveis dos tipos de instrucoes
int RRR,
    RRI,
    BRC,
    CALL,
    ARI1=0,
    LOG1=0,
    SET1=0,
    MEM1=0,
    JMPL1=0,
    RET1=0,
    ADDI1=0,
    ADDUI1=0,
    SUBI1=0,
    SUBUI1=0,
    ANDI1=0,
    ORI1=0,
    XORI1=0,
    NANDI1=0,
    NORI1=0,
    XNORI1=0,
    LSLI1=0,
    LSRI1=0,
    ASRI1=0,
    LUI1=0,
    JMP1=0,
    BEQ1=0,
    BNE1=0,
    BLT1=0,
    BLTU1=0,
    BLE1=0,
    BLEU1=0,
    BGT1=0,
    BGTU1=0,
    BGE1=0,
    BGEU1=0,
    BRT1=0,
    BRF1=0,
    CALL1=0,
    num_ciclos,
    numero;
;

// Procedimento de apresentacao na tela
void show(void)
{
   int total=0;
   float T_RRR, T_RRI, T_BRC, T_CALL, cpi;

   total  = RRR + RRI + BRC + CALL;
   T_RRR  = ((float) RRR / total)  * 100;
   T_RRI  = ((float) RRI / total)  * 100;
   T_BRC  = ((float) BRC / total)  * 100;
   T_CALL = ((float) CALL / total) * 100;
   cpi    = (float)  clock / total_inst;

   printf("\n\n\n\n");
   printf("\nReg [00] - %08lX    Reg [01] - %08lX         Distribuicao das Instrucoes",reg[0],reg[1]);
   printf("\nReg [02] - %08lX    Reg [03] - %08lX",reg[2],reg[3]);
   printf("\nReg [04] - %08lX    Reg [05] - %08lX         MIF_RRR  = %2.2f %%",reg[4],reg[5],T_RRR);
   printf("\nReg [06] - %08lX    Reg [07] - %08lX         MIF_RRI  = %2.2f %%",reg[6],reg[7],T_RRI);
   printf("\nReg [08] - %08lX    Reg [09] - %08lX         MIF_BRC  = %2.2f %%",reg[8],reg[9],T_BRC);
   printf("\nReg [10] - %08lX    Reg [11] - %08lX         MIF_CALL = %2.2f %%",reg[10],reg[11],T_CALL);
   printf("\nReg [12] - %08lX    Reg [13] - %08lX",reg[12],reg[13]);
   printf("\nReg [14] - %08lX    Reg [15] - %08lX",reg[14],reg[15]);
   printf("\nReg [16] - %08lX    Reg [17] - %08lX         Total de Inst: %d",reg[16],reg[17],num_inst);
   printf("\nReg [18] - %08lX    Reg [19] - %08lX",reg[18],reg[19]);
   printf("\nReg [20] - %08lX    Reg [21] - %08lX",reg[20],reg[21]);
   printf("\nReg [22] - %08lX    Reg [23] - %08lX         PC    = %d",reg[22],reg[23],pc);
   printf("\nReg [24] - %08lX    Reg [25] - %08lX         Clock = %d",reg[24],reg[25],clock);
   printf("\nReg [26] - %08lX    Reg [27] - %08lX",reg[26],reg[27]);
   printf("\nReg [28] - %08lX    Reg [29] - %08lX         CPI   = %2.2f",reg[28],reg[29],cpi);
   printf("\nReg [30] - %08lX    Reg [31] - %08lX",reg[30],reg[31]);
   printf("\nInstrucao buscada [%08lX]",memoria[pc]);
}

// Estagio de decodificacao
void decode (void)
{
  di.op1 = (fi.inst >> 30) & 0x03L;
  di.op2 = (fi.inst >> 26) & 0x0fL;
  switch (di.op1) {
     case MIF_RRR:
		 di.op3 = (fi.inst >> 6) & 0x1fL;
// Testando dependencia de dados
// Reg fonte 1

		if (((fi.inst >> 21) & 0x1fL) == fw.id1)
		   di.rs1 = fw.data1;
		else
		if (((fi.inst >> 21) & 0x1fL) == fw.id2)
		   di.rs1 = fw.data2;
                else
		   di.rs1 = reg[(fi.inst >> 21) & 0x1fL];
// Reg fonte 2

		if (((fi.inst >> 16) & 0x1fL) == fw.id1)
			di.rs2 = fw.data1;
			else
		if (((fi.inst >> 16) & 0x1fL) == fw.id2)
		   di.rs2 = fw.data2;
		else
		   di.rs2 = reg[(fi.inst >> 16) & 0x1fL];
		di.rd  = (fi.inst >> 11) & 0x1fL;
		break;

    case MIF_RRI:
// Testando dependencia de dados
		if (((fi.inst >> 21) & 0x1fL) == fw.id1)
		   di.rs1 = fw.data1;
		else
		   if (((fi.inst >> 21) & 0x1fL) == fw.id2)
		      di.rs1 = fw.data2;
		   else
		      di.rs1 = reg[(fi.inst >> 21) & 0x1fL];
		   di.rd   = (fi.inst >> 16) & 0x1fL;
		   di.uimm = ((fi.inst & 0xffffL) & 0x0000FFFFL);

		   if (((fi.inst & 0xffffL) & 0x00008000L) == 0)
			di.simm = ((fi.inst & 0xffffL) & 0x0000FFFFL);
		   else
			di.simm = ((fi.inst & 0xffffL) | 0xFFFF0000L);
		   break;

    case MIF_BRC:
// Testando dependencia de dados
// Reg fonte 1

		if (((fi.inst >> 21) & 0x1fL) == fw.id1)
			di.rs1 = fw.data1;
		else
		   if (((fi.inst >> 21) & 0x1fL) == fw.id2)
			di.rs1 = fw.data2;
		   else
			di.rs1 = reg[(fi.inst >> 21) & 0x1fL];
// Reg fonte 2

		if (((fi.inst >> 16) & 0x1fL) == fw.id1)
			di.rs2 = fw.data1;
		else
		   if (((fi.inst >> 16) & 0x1fL) == fw.id2)
		      di.rs2 = fw.data2;
		   else
		      di.rs2 = reg[(fi.inst >> 16) & 0x1fL];
		if (((fi.inst & 0xffffL) & 0x00008000L) == 0)
			di.simm = ((fi.inst & 0xffffL) & 0x0000FFFFL);
		else
			di.simm = ((fi.inst & 0xffffL) | 0xFFFF0000L);
		break;

    case MIF_CALL:
	      di.uimm = fi.inst & 0x3fffffffL;
	      break;
    default:
	      printf("decode: unknown instruction format\n");
	      exit(0);
  }
  di.flag = fi.flag;
  di.pc = fi.pc;
}

void execute(void)
{
int i,flag=1;

switch (di.op1)
   {
   case MIF_RRR:
     RRR++;

     switch (di.op2)
     {
     case ARITH:
	ARI1++;
	switch (di.op3)
		{
		case ADD:
			ex.res = (signed)di.rs1 + (signed)di.rs2;
			break;

		case ADDU:
			ex.res = di.rs1 + di.rs2;
			break;

		case SUB:
			ex.res = (signed)di.rs1 - (signed)di.rs2;
			break;

		case SUBU:
			ex.res = di.rs1 - di.rs2;
			break;

		case MUL:
			ex.res = (signed)di.rs1 * (signed)di.rs2;
			break;

		case MULU:
			ex.res = di.rs1 * di.rs2;
			break;
		}
	break;

     case LOGIC:
	LOG1++;
	switch (di.op3)
		{
		case AND:
			ex.res = di.rs1 & di.rs2;
			break;

		case OR:
        		ex.res = di.rs1 | di.rs2;
   			break;

		case XOR:
   			ex.res = di.rs1 ^ di.rs2;
   			break;

		case NAND:
			ex.res = ~(di.rs1 & di.rs2);
			break;

		case NOR:
			ex.res = ~(di.rs1 | di.rs2);
			break;

		case XNOR:
			ex.res = ~(di.rs1 ^ di.rs2);
			break;

		case NOT:
			ex.res = ~di.rs1;
			break;

		case LSL:
			ex.res = di.rs1 << di.rs2;
			break;

		case LSR:
			ex.res = di.rs1 >> di.rs2;
			break;

		case ASR:
			if (di.rs2 > 0)
				if ((di.rs1 >> 31) & 0x01F)
				    for (i=1;i <= di.rs2;i++)
					ex.res = (di.rs1 >> 1) | 0x80000000L;
                                else
					ex.res = di.rs1 >> di.rs2;
                        else
                           ex.res = di.rs1;
			break;
		}
	break;

     case SET:
	SET1++;
	switch (di.op3)
		{
		case SEQ:
			if (di.rs1 == di.rs2)
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SNE:
			if (di.rs1 != di.rs2)
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SLT:
			if (((signed)(di.rs1)) < ((signed)(di.rs2)))
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SLTU:
			if (di.rs1 < di.rs2)
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SLE:
			if (((signed)(di.rs1)) <= ((signed)(di.rs2)))
				ex.res = 1;
			else
   				ex.res = 0;
			break;

		case SLEU:
			if (di.rs1 <= di.rs2)
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SGT:
			if (((signed)(di.rs1)) > ((signed)(di.rs2)))
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SGTU:
			if (di.rs1 > di.rs2)
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SGE:
			if (((signed)(di.rs1)) >= ((signed)(di.rs2)))
				ex.res = 1;
			else
				ex.res = 0;
			break;

		case SGEU:
			if (di.rs1 >= di.rs2)
				ex.res = 1;
			else
				ex.res = 0;
			break;
		}
	break;

     case MEM:
	MEM1++;
	switch (di.op3)
		{
		case LDW:
			if (di.rs1 <= MAX)
			   ex.res = memoria[di.rs1];
			else
			   {
			    printf("\nERRO: Instrucao Load acessou posicao [%d] da memoria\n",di.rs1);
			    printf("PC = %d    Clock = %d\n",di.pc,clock);
   			    printf("Instrucao [%X]\n",memoria[di.pc]);
			    exit(1);
			   }
			break;

		case STW:
			if (di.rs1 <= MAX)
			   memoria[di.rs1] = di.rs2;
			else
			   {
			    printf("\nERRO: Instrucao Store acessou posicao [%d] da memoria\n",di.rs1);
			    printf("PC = %d    Clock = %d\n",di.pc,clock);
			    printf("Instrucao [%X]\n",memoria[di.pc]);
			    exit(1);
			   }
			flag = 0;
			break;
		}
	break;

     case JMPL:
	JMPL1++;
	if (di.rs1 > MAX)
	   {
	    printf("\nERRO: Instrucao JPML acessou posicao de memoria [%d]\n",di.rs1);
	    printf("PC = %d    Clock = %d\n",di.pc,clock);
	    printf("Instrucao [%X]\n",memoria[di.pc]);
	    exit(1);
	   }
	ex.res  = pc + 2;
	pc = di.rs1;
	break;

     case RET:
	RET1++;
	if (reg[31] > MAX)
	   {
	    printf("\nERRO: Instrucao RET acessou posicao de memoria [%d]\n",reg[31]);
	    printf("PC = %d    Clock = %d\n",di.pc,clock);
	    printf("Instrucao [%X]\n",memoria[di.pc]);
	    exit(1);
	   }
	pc = reg[31];
	flag = 0;
	break;
   }
   break;

   case MIF_RRI:
      RRI++;
     switch (di.op2)
     {
     case ADDI:
	ADDI1++;
	ex.res = (signed)di.rs1 + di.simm;
	break;

     case ADDUI:
	ADDUI1++;
	ex.res = di.rs1 + di.uimm;
	break;

     case SUBI:
	SUBI1++;
	ex.res = (signed)di.rs1 - di.simm;
	break;

     case SUBUI:
	SUBUI1++;
	ex.res = di.rs1 - di.uimm;
	break;

     case ANDI:
	ANDI1++;
	ex.res = di.rs1 & di.uimm;
	break;

     case ORI:
	ORI1++;
	ex.res = di.rs1 | di.uimm;
	break;

     case XORI:
	XORI1++;
	ex.res = di.rs1 ^ di.uimm;
	break;

     case NANDI:
	NANDI1++;
	ex.res = ~(di.rs1 & di.uimm);
	break;

     case NORI:
	NORI1++;
	ex.res = ~(di.rs1 | di.uimm);
	break;

     case XNORI:
	XNORI1++;
	ex.res = ~(di.rs1 ^ di.uimm);
	break;

     case LSLI:
	LSLI1++;
	ex.res = di.rs1 << di.uimm;
	break;

     case LSRI:
	LSRI1++;
	ex.res = di.rs1 >> di.uimm;
	break;

     case ASRI:
	ASRI1++;
	if (di.uimm != 0)
        	if ((di.rs1 >> 31) & 0x01F)
                	for (i=1;i <= di.uimm;i++)
                               ex.res = (di.rs1 >> 1) | 0x80000000L;
                else
                   	ex.res = di.rs1 >> di.uimm;
        else
           ex.res = di.rs1;
	break;

     case LUI:
	LUI1++;
	if (di.uimm >= MAX)
	   {
	    printf("\nERRO: Instrucao LUI acessou posicao de memoria [%d]\n",di.uimm);
	    printf("PC = %d    Clock = %d\n",di.pc,clock);
	    printf("Instruction [%X]\n",memoria[di.pc]);
	    exit(1);
	   }
	ex.res = (di.uimm << 16) & 0xffff0000;
	break;

     case JMP:
	JMP1++;
	pc = di.pc + di.simm;
	if (pc >= MAX)
	   {
	    printf("\nERRO: Instrucao JPML acessou posicao de memoria [%d]\n",pc);
	    printf("PC = %d    Clock = %d\n",di.pc,clock);
	    printf("Instruction [%X]\n",memoria[di.pc]);
	    exit(1);
	   }
	flag = 0;
	break;
     }
   break;

   case MIF_BRC:
     BRC++;
     flag = 0;

     switch (di.op2)
     {
     case BEQ:
	BEQ1++;
	if (di.rs1 == di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BEQ acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	    }
	break;

     case BNE:
	BNE1++;
	if (di.rs1 != di.rs2)
	   {
	   pc = di.pc + di.simm;
	   if (pc >= MAX)
		{
		printf("\nERRO: Instrucao BNE acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BLT:
	BLT1++;
	if ((signed)di.rs1 < (signed)di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BLT acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	    }
	break;

     case BLTU:
	BLTU1++;
	if (di.rs1 <= di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BLTU acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BLE:
	BLE1++;
	if ((signed)di.rs1 <= (signed)di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BLE acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BLEU:
	BLEU1++;
	if (di.rs1 <= di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BLEU acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instructao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BGT:
	BGT1++;
	if ((signed)di.rs1 > (signed)di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BGT acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BGTU:
	BGTU1++;
	if (di.rs1 > di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BGTU acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BGE:
   	BGE1++;
	if ((signed)di.rs1 >= (signed)di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BGE acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BGEU:
	BGEU1++;
	if (di.rs1 >= di.rs2)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BGEU acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BRT:
	BRT1++;
	if (di.rs1 == 1)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BRT acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;

     case BRF:
	BRF1++;
	if (di.rs1 == 0)
	   {
	    pc = di.pc + di.simm;
	    if (pc >= MAX)
	       {
		printf("\nERRO: Instrucao BRF acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
	   }
	break;
     }
     break;

   case MIF_CALL:
      CALL++;
      CALL1++;
      ex.res = di.pc + 2;
      di.rd  = 31;
       if (di.uimm >= MAX)
	       {
		printf("\nERRO: Instrucao CALL acessou posicao de memoria [%d]\n",di.rs1);
		printf("PC = %d    Clock = %d\n",di.pc,clock);
		printf("Instrucao [%X]\n",memoria[di.pc]);
		exit(1);
	       }
      pc = di.uimm;
      break;

   default:
      printf("execute: error falta\n");
      exit(0);
  }

  ex.flag = flag; // recebe 0 quando forem saltos que nao alteram regs
  ex.rd = di.rd;

// Atualizando forwarding

fw.id2   = fw.id1;
fw.data2 = fw.data1;
fw.id1   = di.rd;
if (di.rd == 0)
	ex.res = 0;
fw.data1 = ex.res;
}

void result(void)
{
   if (ex.rd != 0)
      reg[ex.rd] = ex.res;

   total_inst++;
}

void load(char *entrada)
{
   FILE *fp;
   char ch;
   int i=0,desloc=0;
   unsigned long int temp;

   if((fp=fopen(entrada,"r"))==NULL)
      {
       fprintf(stderr,"ERROR: Nao pode abrir arquivo [%s]\n",entrada);
       exit(1);
      }
   else
      {
       do{
	  for(desloc=28;desloc>=0;desloc=desloc-4)
	     {
		temp = 0;
		do{
		   ch=getc(fp);
		}while(!isxdigit(ch) && (ch!=EOF));

	      if (ch!=EOF)

	       switch (ch)
		  {
		  case '0':
		     temp = 0;
		     break;

		   case '1':
   		      temp = 1;
		      break;

		   case '2':
		      temp = 2;
		      break;

		   case '3':
		      temp = 3;
		      break;

		   case '4':
		      temp = 4;
		      break;

		   case '5':
   		      temp = 5;
		      break;

		   case '6':
		      temp = 6;
		      break;

		   case '7':
		      temp = 7;
		      break;

		   case '8':
		      temp = 8;
		      break;

		   case '9':
		      temp = 9;
		      break;

		   case 'A':
		      temp = 10;
		      break;

		   case 'B':
		      temp = 11;
		      break;

		   case 'C':
		      temp = 12;
		      break;

		   case 'D':
		      temp = 13;
		      break;

		   case 'E':
		      temp = 14;
		      break;

		   case 'F':
		      temp = 15;
		      break;
		  }
	       memoria[i] = (temp << desloc) | memoria[i];
	     }
	  i++;
       }while(ch!=EOF);

       num_inst=i-1;
       fclose(fp);
      }
}

void fetch(void)
{
   fi.pc=pc;
   fi.inst = memoria[pc];
   fi.flag=1;
}

void regiao(void)
{
   unsigned long int inicial, final, i;
   char tela;

   printf("\n\n\nEntre com a posicao inical : ");
   scanf("%u",&inicial);
   printf("Entre com a posicao final  : ");
   scanf("%u",&final);

   if ((signed)inicial > (signed)final)
      printf("\nPosicao inicial nao pode ser maior que posicao final");
   else
      {
       if (inicial == final)
	  printf("\nPosicao inicial nao pode ser igual a posicao final");
       else
	  {
	   if (((signed)final) < 0)
	      printf("\nPosicao final nao pode ser negativo");
	   else
	      {
	       if ((signed)inicial < 0)
		  printf("\nPosicao inicial nao pode ser negativo");
	       else
		  {
		   if ((signed)final > MAX)
		      {
		       printf("\nPosicao final maior que memoria disponivel");
		       printf("\nTamanho disponivel de memoria - 32kb ou  %d posicoes",MAX);
		      }
		   else
		      { 
			for (i=inicial;i<=final;i++)
			   printf("\nPosicao %4d - Conteudo da memoria [%X]",i,memoria[i]);
		      }
		  }
	      }
          }
       }

   printf("\n\nPrescione qualquer tecla para continuar");
   scanf("%c",&tela);
   scanf("%c",&tela);
}

void distribuicao(void)
{
   int total=0;
   float t1,t2;
   char tela;

   total = ARI1+LOG1+SET1+ MEM1+JMPL1+RET1+ADDI1+ADDUI1+SUBI1+SUBUI1+ANDI1+ORI1+XORI1+NANDI1+
	    NORI1+XNORI1+LSLI1+LSRI1+ASRI1+LUI1+JMP1+BEQ1+BNE1+BLT1+BLTU1+BLE1+BLEU1+BGT1+
	    BGTU1+BGE1+BGEU1+BRT1+BRF1+CALL1;

   printf("\n\n Distribuicao dos tipos de instrucoes executadas\n");

   t1 = ((float) ARI1 / total) * 100;
   t2 = ((float) LOG1 / total) * 100;
   printf("\nARITH = %06.2f %%              LOGIC = %06.2f %%",t1,t2);

   t1 = ((float) SET1 / total) * 100;
   t2 = ((float) MEM1 / total) * 100;
   printf("\nSET   = %06.2f %%              MEM   = %06.2f %%",t1,t2);

   t1 = ((float) JMPL1 / total) * 100;
   t2 = ((float) RET1 / total) * 100;
   printf("\nJMPL  = %06.2f %%              RET   = %06.2f %%",t1,t2);

   t1 = ((float) ADDI1 / total) * 100;
   t2 = ((float) ADDUI1 / total) * 100;
   printf("\nADDI  = %06.2f %%              ADDUI = %06.2f %%",t1,t2);

   t1 = ((float) SUBI1 / total) * 100;
   t2 = ((float) SUBUI1 / total) * 100;
   printf("\nSUBI  = %06.2f %%              SUBUI = %06.2f %%",t1,t2);

   t1 = ((float) ANDI1 / total) * 100;
   t2 = ((float) ORI1 / total) * 100;
   printf("\nANDI  = %06.2f %%              ORI   = %06.2f %%",t1,t2);

   t1 = ((float) XORI1 / total) * 100;
   t2 = ((float) NANDI1 / total) * 100;
   printf("\nXORI  = %06.2f %%              NANDI = %06.2f %%",t1,t2);

   t1 = ((float) NORI1 / total) * 100;
   t2 = ((float) XORI1 / total) * 100;
   printf("\nNORI  = %06.2f %%              XORI  = %06.2f %%",t1,t2);

   t1 = ((float) LSLI1 / total) * 100;
   t2 = ((float) LSRI1 / total) * 100;
   printf("\nLSLI  = %06.2f %%              LSRI  = %06.2f %%",t1,t2);

   t1 = ((float) ASRI1 / total) * 100;
   t2 = ((float) LUI1 / total) * 100;
   printf("\nASRI  = %06.2f %%              LUI   = %06.2f %%",t1,t2);

   t1 = ((float) JMP1 / total) * 100;
   t2 = ((float) BEQ1 / total) * 100;
   printf("\nJMP   = %06.2f %%              BEQ   = %06.2f %%",t1,t2);

   t1 = ((float) BNE1 / total) * 100;
   t2 = ((float) BLT1 / total) * 100;
   printf("\nBNE   = %06.2f %%              BLT   = %06.2f %%",t1,t2);

   t1 = ((float) BLTU1 / total) * 100;
   t2 = ((float) BLE1 / total) * 100;
   printf("\nBLTU  = %06.2f %%              BLE   = %06.2f %%",t1,t2);

   t1 = ((float) BLEU1 / total) * 100;
   t2 = ((float) BGT1 / total) * 100;
   printf("\nBLEU  = %06.2f %%              BGT   = %06.2f %%",t1,t2);

   t1 = ((float) BGTU1 / total) * 100;
   t2 = ((float) BGE1 / total) * 100;
   printf("\nBGTU  = %06.2f %%              BGE   = %06.2f %%",t1,t2);

   t1 = ((float) BGEU1 / total) * 100;
   t2 = ((float) BRT1 / total) * 100;
   printf("\nBGEU  = %06.2f %%              BRT   = %06.2f %%",t1,t2);

   t1 = ((float) BRF1 / total) * 100;
   t2 = ((float) CALL1 / total) * 100;
   printf("\nBRF   = %06.2f %%              CALL  = %06.2f %%",t1,t2);

   printf("\n\nPressione qualquer tecla para continuar\n");
   scanf("%c",&tela);
   scanf("%c",&tela);
}

void ciclos(void)
{
   char teste;

   printf("\n\n\nEntre com o numero de ciclos para executar : ");
   scanf("%d",&num_ciclos);

   if ((signed)num_ciclos < 0)
      {
       printf("\nNumero de ciclos nao pode ser negativo");
       printf("\n\nPrescione [ENTER] para continuar");
       num_ciclos=0;
       teste = getchar();
       teste = getchar();
      }
   else
      {
       if (num_ciclos ==0)
	  {
	   num_ciclos=0;
	   printf("\nNumero de ciclos nao pode ser zero");
	   printf("\n\nPrescione [ENTER] para continuar");
	   teste = getchar();
	   teste = getchar();
	  }
      }
   numero=1;
}

void clean(void)
{
   unsigned long int i;

   for (i=0;i<MAX;i++)  //Limpa a memoria
       memoria[i]=0;

   for (i=0;i<32;i++)   //Limpa registradores
      reg[i]=0;

   clock=1;
   pc=0;
   RRR=0;
   RRI=0;
   BRC=0;
   CALL=0;
   numero=1;
   num_ciclos=0;
   total_inst=0;

      // Inicializando forwarding
   fw.id1   = 32; //registrador inexistente
   fw.id2   = 32;
   fw.data1 = 0;
   fw.data2 = 0;
}

int main(int argc, char *argv[])
{
   char opcao;

   if(argc!=2)
      {
       fprintf(stderr,"\nSintaxe :  pipeline <arquivo_entrada>\n\n");
       exit(1);
      }
   else
      {
	clean();
	load(argv[1]);
      }

   do {
       if (ex.flag)
	  result();

       if (di.flag)
	  execute();

       if (fi.flag)
	  decode();

       fetch();
       show();
       if (numero >= num_ciclos)
	  {
	   printf("\n\nEscolha uma das opcoes:");
	   printf("\n(ENTER) - Executar proximo ciclo (D) - Distribuicao das instrucoes intra-grupo");
	   printf("\n(M) - Mostrar regiao da memoria (C) - Numero de ciclos a serem executados");
	   printf("\n(S) - Sair\n");
	   opcao = getchar();
	   opcao = toupper(opcao);
	   switch(opcao)
	      {
		 case 'D':
		    distribuicao();
		    break;

		 case 'M':
		    regiao();
		    break;

		 case 'C':
		    ciclos();
		    break;

		 case 'S':
		    exit(1);
		    break;

		 default :
		    clock++;
		    pc++;
	       }
	  }
       else
	  {
	   numero++;
	   clock++;
	   pc++;
	  }
    }while(pc < num_inst);
    do
    {
    printf("\n\nPara visualizar alguma regiao da memoria digite M,\n");
    printf("distribuicao de instrucoes intra-grupo D ou sair S\n");
    opcao = getchar();
    opcao = toupper(opcao);
    if (opcao == 'M')
       regiao();
    else
       if (opcao == 'D')
       	  distribuicao();
    }
    while (opcao != 'S'); 
}



