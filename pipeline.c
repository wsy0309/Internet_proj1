#include<stdio.h>
#include<string.h>

typedef struct inst_t{
	unsigned int opcode;
	unsigned int rs, rt, rd;
	unsigned int shamt, func;
	unsigned int imm;
	unsigned int imm_j;

	//control signal
	int Jump;   //jump
	int Branch;   // branch
	int Rupdate;  //register 업데이트
	int R_update; //그중 r type
	int I_update; //그중 i type
	int Mem;	 //mem접근 모두
	int MemRead; //그 중 load
	int MemWrite; //그중 store
}inst_t;

typedef struct IF_latch{
	unsigned int valid;
	unsigned int pc4;
	unsigned int inst;
}IF_latch;

typedef struct ID_latch{
	unsigned int valid;
	unsigned int pc4;
	unsigned int inst;
	inst_t i;
	unsigned int rs_v;
	unsigned int rt_v;
	unsigned int npc;
	unsigned int rd;
	int s_imm;
}ID_latch;

typedef struct EX_latch{
	unsigned int valid;
	unsigned int pc4;
	unsigned int rt_v;
	unsigned int rd;
	unsigned int npc;
	unsigned int ALU_res;   //rt_v랑 simm mux????
	unsigned int inst;
	inst_t i;
}EX_latch;

typedef struct MEM_latch{
	unsigned int valid;
	unsigned int pc4;
	unsigned int inst;
	unsigned int data;
	unsigned int rd;
	unsigned int ALU_res;
	inst_t i;
}MEM_latch;


//some reg. name aliases
#define zero 0
#define at   1
#define v0   2
#define v1   3
#define a0   4
#define a1   5
#define a2   6
#define a3   7
#define t0   8
#define t1   9
#define t2   10
#define t3   11
#define t4   12
#define t5   13
#define t6   14
#define t7   15
#define s0   16
#define s1   17
#define s2   18
#define s3   19
#define s4   20
#define s5   21
#define s6   22
#define s7   23
#define t8   24
#define t9   25
#define k0   26
#define k1   27
#define gp   28
#define sp   29
#define fp   30
#define ra   31

// some opcode deifinitions
// JUMP is set for the following insts.
#define JR		0x0
#define JR_FUNC	0x8
#define J		0x2
#define JAL 	0x3

// IADD is set when
#define ADDI 	0x8
#define ADDIU 	0x9
#define ANDI 	0xc
#define ORI 	0xd
#define SLTI 	0xa
#define SLTIU	0xb

// MEM is set when
	// LOAD
#define LBU 	0x24
#define LHU 	0x25
#define LL 		0x30
#define LW 		0x23
#define LUI     0xf
	//STORE
#define SB 		0x28
#define SH 		0x29
#define SW		0x2b

// BRANCH is set when
#define BEQ 	0x4
#define BNE		0x5


//GLOBAL//
#define MAX_REG 32
#define MAX_MEM 16*1024*1024/sizeof(int)

IF_latch if_latch[2];            //latch stage output(0)/input(1)
ID_latch id_latch[2];            //decode stage output(0)/input(1)
EX_latch ex_latch[2];			 //execution stage output(0)/input(1)
MEM_latch mem_latch[2];			 //load_store stage output(0)/input(1)

int cycle;                       //cycle count
int mem_op;						 //memory operation count
int reg_op;						 //writeback count
int branch_op;					 //taken branch count
int not_branch_op;				 //non taken branch count
int jump_op;					 //jump count

//Memory, register, pc
unsigned int Mem[MAX_MEM];
unsigned int reg[MAX_REG];
unsigned int pc;

void machine_initialize(IF_latch *if_out);
void load_program(FILE *fd);
int fetch_inst(IF_latch *if_out, IF_latch *if_in, ID_latch *id_in, EX_latch *ex_in); 
void decode_inst(ID_latch *id_out, IF_latch *if_in, IF_latch *if_out);
void execute_inst(ID_latch *id_in, EX_latch *ex_out, EX_latch *ex_in, MEM_latch *mem_in, IF_latch *if_out);
void load_store(EX_latch *ex_in, MEM_latch *mem_out, MEM_latch *mem_in);
int write_back(MEM_latch *mem_in);
void print_inst(struct inst_t *inst);
void statistics();

/******************************************************
update_latch : stage의 결과값을 input latch로 update
********************************************************/

void update_latch(IF_latch if_latch[2], ID_latch id_latch[2], EX_latch ex_latch[2], MEM_latch mem_latch[2]){
	if_latch[1] = if_latch[0];
	id_latch[1] = id_latch[0];
	ex_latch[1] = ex_latch[0];
	mem_latch[1] = mem_latch[0];
	// latch[0] : output,  latch[1] : input
} 

void clock(){
	cycle++;
	printf("=============== cycle %d ================= \n",cycle);
}

int main(){
	int ret = 0;
	FILE* fd = NULL;	
	//file open
    fd = fopen("simple2.bin","r");
	//초기값 설정
	machine_initialize(&if_latch[0]);
	load_program(fd);
	//file close
    fclose(fd);

	do{
		//cycle 증가 & 출력
		clock();
		//pc=-1 && 마지막 inst writeback후 return -1   => 프로그램 종료
		ret = write_back(&mem_latch[1]);
		if(ret == -1) break;
		fetch_inst(&if_latch[0], &if_latch[1], &id_latch[1], &ex_latch[1]);		
		decode_inst(&id_latch[0], &if_latch[1], &if_latch[0]);
		execute_inst(&id_latch[1], &ex_latch[0], &ex_latch[1], &mem_latch[1], &if_latch[0]);
		load_store(&ex_latch[1], &mem_latch[0], &mem_latch[1]);		
		update_latch(if_latch, id_latch, ex_latch, mem_latch);		
	}while(ret >= 0);
	//실행 결과 출력
	statistics();
	return 0;
}

void print_inst(inst_t *inst){
	//decode 결과 출력
	printf("Dec : opcode : %x, rs : %x, rt : %x, rd : %x, shamt : %x, func : %x, imm : %x, imm_j : %x \n",
		inst->opcode,inst->rs,inst->rt,inst->rd,inst->shamt,inst->func,inst->imm,inst->imm_j);
}



/******************************************************************
machine_initialize : 프로그램 시작시 set initial value
 -> pc, fp, sp, ra, cycle, stage_op, valid(fetch[0])
*******************************************************************/

void machine_initialize(IF_latch *if_out){
	pc = 0;	
	reg[fp] = 0x400000; 
	reg[sp] = 0x400000;
	reg[ra] = -1;
	cycle = 0;
	mem_op = 0;
    reg_op = 0;
	branch_op = 0;
	not_branch_op = 0;
	jump_op = 0;
	if_out->valid = 1;	
}



/********************************************************
load_program : file 내용 Mem에 저장
- fread사용하여 1byte씩 불러옴
- 시프트 연산자(<<) 사용하여 instruction order 설정
*********************************************************/

void load_program(FILE *fd){
	int data1 = 0, data2 = 0, data3 = 0, data4 = 0;
    int data = 0;
    int res = 1;
    int num = 0;
	while(res == 1){
		//file 다 읽은 후 break
        res = fread(&data1,sizeof(unsigned char),1,fd);       
        if(res != 1)    break;
        res = fread(&data2,sizeof(unsigned char),1,fd);        
        if(res != 1)    break;
        res = fread(&data3,sizeof(unsigned char),1,fd);		
        if(res != 1)    break;
        res = fread(&data4,sizeof(unsigned char),1,fd);       
        if(res != 1)    break;
		//instruction 정리
        data = data1<<24 | data2<<16 | data3<<8 | data4;
       
		//Memory에 저장
        Mem[num] = data;
        printf("Men[%x] val : 0x%08x \n",num,Mem[num]);
        num++;		
	}
}

/*
void load_program(FILE *fd){
    
    int index = 0;
    int data_[4];
    while(!feof(fd)){
        
        for(int i = 0;i<4;i++){
            fread(&data_[i], sizeof(unsigned char), 1, fd);
        }
        
        Mem[index] = data_[0]<<24|data_[1]<<16|data_[2]<<8|data_[3];
        printf("load Mem[%d] : 0x%x\n",index,Mem[index]);
        index++;
    }
}
*/
/***********************************************************************
fetch_inst : inst에 따라 change pc
 - decode stage inst = jump : pc에 jump address할당
 - execute stage inst = branch : pc에 branch address할당
 - cycle = 1 : pc 변하지 않음
 - pc = -1 : fetch invalid 시킨 후 inst = -1 할당
 - else : pc +4
***********************************************************************/

int fetch_inst(IF_latch *if_out, IF_latch *if_in, ID_latch *id_in, EX_latch *ex_in){
	
	if(if_out->valid){		
		if(id_in->i.Jump)			
			pc = id_in->npc;
		else if(ex_in->i.Branch)			
			pc = ex_in->npc;
		else{
			if(cycle != 1)
				pc = pc + 4;
		}
		if(pc == -1){
			if_out->valid = 0;
			if_out->inst = -1;
			return 0;
		}
		else{
		//inst 할당 (pc/4 : index)
		if_out->inst = Mem[pc/4];
		if_out->pc4	= pc;				
		printf("Fet : (pc : 0x%08x): %x \n",pc,Mem[pc/4]);
		}
	}
	return 0;
}

/**************************************************************************
decode_inst : inst decode 후 결과값 id_latch[0]에 저장  
 - inst = branch : decode 무조건 valid
	-> else : fetch output 받음
 - inst 각각 control signal set
 - decode 결과 출력
 - count Jump operation
***************************************************************************/

void decode_inst(ID_latch *id_out, IF_latch *if_in, IF_latch *if_out){
	int s_bit;
	inst_t inst;
	id_out->inst = if_in->inst;
	//inst가 branch의 경우 fetch[0]의 valid값과 관계없이 decode는 항상 실행되어야 함
	if(id_out->i.opcode == BEQ || id_out->i.opcode == BNE)
		id_out->valid = 1;
	else
		id_out->valid = if_in->valid;

	if(id_out->valid){
		memset(&inst,0,sizeof(inst)); //inst 초기화
		id_out->pc4 = if_in->pc4;
		int temp = id_out->inst;
		//inst decode
		inst.opcode = (temp & 0xfc000000)>>26;
		inst.rs = (temp & 0x3e00000)>>21;
		inst.rt = (temp & 0x1f0000)>>16;
		inst.rd = (temp & 0xf800)>>11;
		inst.shamt = (temp & 0x7c0)>>6;
		inst.func = temp & 0x3f;
		inst.imm = temp & 0xffff;
		inst.imm_j = temp & 0x3ffffff;		

		//control signal
		if(inst.opcode == J || (inst.opcode == JR && inst.func == JR_FUNC) || inst.opcode == JAL){
			//count jump operation
			jump_op++;
			inst.Jump = 1;			
		}
		//R-type
		if(inst.opcode == 0){
			inst.Rupdate = 1;
			inst.R_update = 1;
		}
		//I-type 중 reg operation
		if(inst.opcode == ADDI || inst.opcode == LUI || inst.opcode == ADDIU || inst.opcode == ANDI || inst.opcode == ORI || inst.opcode == SLTI || inst.opcode == SLTIU){
			inst.Rupdate = 1;
			inst.I_update = 1;
		}
		//load
		if(inst.opcode == LBU || inst.opcode == LHU || inst.opcode == LL || inst.opcode == LW){
			inst.Mem = 1;
			inst.MemRead = 1;
			inst.Rupdate = 1;
			inst.I_update = 1;
		}
		//store
		if(inst.opcode == SW || inst.opcode == SB || inst.opcode == SH){
			inst.Mem = 1;
			inst.MemWrite = 1;
		}
		//branch
		if(inst.opcode == BEQ || inst.opcode == BNE){
			if_out->valid = 0;			
		}
	
		id_out->rs_v = reg[inst.rs];   //rs value
		id_out->rt_v = reg[inst.rt];   //rt value
		if(inst.R_update)
			id_out->rd = inst.rd;
		if(inst.I_update)
			id_out->rd = inst.rt;            //rd value(I or R type)
		//JR : npc = rs value & register update
		if((inst.opcode == JR && inst.func == JR_FUNC)){   
			id_out->npc = id_out->rs_v;
			inst.Rupdate = 0;
			inst.R_update = 0;
		}
		//else : npc = jump address
		else
			id_out->npc = ((if_in->pc4 + 4) & 0xf0000000) | (inst.imm_j << 2);
		//JAL : register 31에 결과 저장(register update)
		if(inst.opcode == JAL){
			id_out->rd = ra;
			inst.Rupdate = 1;
		}

		s_bit = inst.imm >> 15;
		if(s_bit)
			id_out->s_imm = (short)inst.imm;
		else
			id_out->s_imm = inst.imm;

		id_out->i = inst;
		print_inst(&inst);
	}
}

/***********************************************************************************
execute_inst : ALU계산 후 결과값 ex_latch[0]에 저장
 - data dependency check
     -> distance = 2 일 때, load_store stage의 결과값 이용 dependency 해결
	 -> distance = 1 일 때, execute의 결과값 이용 dependency 해결
	 -> 각각 rt, rs value 모두 체크
  - control dependency check
     -> opcode = Branch 일 때 stall 시킴 (cycle 증가)
	 -> count Branch(taken/not taken) operation
*************************************************************************************/

void execute_inst(ID_latch *id_in, EX_latch *ex_out, EX_latch *ex_in, MEM_latch *mem_in, IF_latch *if_out){
	unsigned int opcode,rs,rd,rt,shamt,func,imm,b_addr,rs_v,rt_v;
	int s_bit;
	ex_out->inst = id_in->inst;
	ex_out->valid = id_in->valid;

	if(ex_out->valid){
		ex_out->i = id_in->i;
		ex_out->pc4 = id_in->pc4;
		ex_out->npc = 0;
		ex_out->rd = id_in->rd;
		ex_out->rt_v = id_in->rt_v;
	
		int s_ext_imm = id_in->s_imm;
		int z_ext_imm = id_in->i.imm;
		opcode = id_in->i.opcode;
		rs = id_in->i.rs;
		rt = id_in->i.rt;
		rd = ex_out->rd;
		rs_v = id_in->rs_v;		
		rt_v = id_in->rt_v;
		shamt = id_in->i.shamt;
		func = id_in->i.func;
		imm = id_in->i.imm;
		s_bit = imm >> 15;

		if(s_bit)
			b_addr = 0xfffc0000 | imm << 2;
		else
			b_addr = imm << 2;
	
		// dependency -> bypassing
		// distance = 2  (dependency 발생시 load_store의 결과값 가져옴)
		if(mem_in->i.Rupdate){
			//check rs value
			if(rs == mem_in->rd)
				rs_v = mem_in->data;
			//check rt value
			if(rt == mem_in->rd){
				rt_v = mem_in->data;
				//바뀐 값 update
				ex_out->rt_v = mem_in->data;
			}
		}
		// distance = 1  (dependency 발생시 execution의 결과값 가져옴)
		if(ex_in->i.Rupdate){
			//check rs value
			if(rs == ex_in->rd)
				rs_v = ex_in->ALU_res;
			//check rt value
			if(rt == ex_in->rd){
				rt_v = ex_in->ALU_res;
				//바뀐 값 update
				ex_out->rt_v = ex_in->ALU_res;
			}
		}

		printf("Exe : ");
		switch(opcode){  
		//R_type instruction
			case 0:
				//funct으로 구별
				switch(func){
				case 0x20:
					ex_out->ALU_res = (int)((int)rs_v + (int)rt_v);
					printf("ADD	R%d (0x%x) = R%d + R%d \n",rd,ex_out->ALU_res,rs,rt);
				break;
				case 0x21:
					ex_out->ALU_res = rs_v + rt_v;				
					printf("ADDU	R%d (0x%x) = R%d + R%d \n",rd,ex_out->ALU_res,rs,rt);
				break;
				case 0x24:
					ex_out->ALU_res = rs_v & rt_v;
					printf("AND	R%d (0x%x) = R%d & R%d \n",rd,ex_out->ALU_res,rs,rt);
				break;
				//change pc value
				case 0x8:			
					printf("JR	PC (0x%x) = R%d \n",pc,rs);				
				break;
				case 0x27:
					ex_out->ALU_res = ~(rs_v | rt_v);
					printf("NOR	R%d (0x%x) = ~(R%d | R%d) \n",rd,ex_out->ALU_res,rs,rt);
				break;
				case 0x25:
					ex_out->ALU_res = rs_v | rt_v;
					printf("OR	R%d (0x%x) = R%d | R%d \n",rd,ex_out->ALU_res,rs,rt);
				break;
				case 0x2a:
					ex_out->ALU_res = ((int)rs_v < (int)rt_v) ? 1:0;
					printf("SLT	R%d (0x%x) = (R%d < R%d)? 1:0 \n",rd,ex_out->ALU_res,rs,rt);
				break;
				case 0x2b:
					ex_out->ALU_res = (rs_v < rt_v) ? 1:0;
					printf("SLTU	R%d (0x%x) = (R%d < R%d)? 1:0 \n",rd,ex_out->ALU_res,rs,rt);
				break;
				//0x00000000일 때 실행, 결과에 영향 주지 않음
				case 0x00:
					ex_out->ALU_res = rt_v << shamt;
					printf("SLL	R%d (0x%x) = R%d << %d \n",rd,ex_out->ALU_res,rt,shamt);
				break;
				case 0x02:
					ex_out->ALU_res = rt_v >> shamt;
					printf("SRL	R%d (0x%x) = R%d >> %d \n",rd,ex_out->ALU_res,rt,shamt);
				break;
				case 0x22:
					ex_out->ALU_res = (int)((int)rs_v - (int)rt_v);
					printf("WB => SUD	R%d (0x%x) = R%d - R%d \n",rd,ex_out->ALU_res,rs,rt);
				break;
				case 0x23:
					ex_out->ALU_res = rs_v - rt_v;
					printf("SUDU	R%d (0x%x) = R%d - R%d \n",rd,ex_out->ALU_res,rs,rt);
				break;
				}
			break;
			//I_type & J_type
			case 0x8:
				ex_out->ALU_res = (int)((int)rs_v + s_ext_imm);
				printf("ADDI	R%d (0x%x) = R%d + (%d) \n",rt,ex_out->ALU_res,rs,s_ext_imm);
			break;
			case 0x9:
				ex_out->ALU_res = rs_v + (unsigned int)s_ext_imm;				
				printf("ADDIU	R%d (0x%x) = R%d + (%d) \n",rt,ex_out->ALU_res,rs,s_ext_imm);
			break;
			case 0xc:
				ex_out->ALU_res = rs_v & z_ext_imm;
				printf("ANDI	R%d (0x%x) = R%d & (%d) \n",rt,ex_out->ALU_res,rs,z_ext_imm);
			break;
			case 0x4:
				if_out->valid = 1;
				//조건 만족 : branch control signal true로 할당				
				if(rs_v == rt_v){					
					ex_out->npc = id_in->pc4 + 4 + b_addr;
					ex_out->i.Branch = 1;
					//count branch operation
					branch_op++;
					cycle++;
					printf("BEG	if(R%d == R%d) PC = %d + 4 + %d \n",rs,rt,pc,b_addr);
				}
				//조건 불만족시 변화 없음
				else{				
					printf("BEG    Condition is not satisfied. \n");
					//count not taken branch operation
					not_branch_op++;
				}		
			break;                                       
			case 0x5:
				if_out->valid = 1;	
				if(rs_v != rt_v){						
					ex_out->npc = id_in->pc4 + 4 + b_addr;
					ex_out->i.Branch = 1;	
					branch_op++;
					cycle++;
					printf(" BNE   if(R%d != R%d) PC = %d + 4 + %d \n",rs,rt,pc,b_addr);
				}
			//조건 불만족시 변화 없음
				else{
					printf("BNE Condition is not satisfied. \n");
					not_branch_op++;
				}
			break;
			case 0x2:
				printf("J	PC = 0x%x \n",id_in->npc);
			break;
			case 0x3:
				//change pc value & calculate res
				ex_out->ALU_res = id_in->pc4 + 8;
				printf("JAL	  R31 = 0x%x + 8  PC = 0x%x \n",pc,id_in->npc);
			break;
			case 0x24:
				ex_out->ALU_res = rs_v + (unsigned int)(s_ext_imm & 0xff);
				printf("LBU	  R%d (0x%x) = Mem[R%d + 0x%x] \n",rt,rt_v,rs,s_ext_imm & 0xff);
			break;
			case 0x25:		
				ex_out->ALU_res = rs_v + (unsigned int)(s_ext_imm & 0xffff);
				printf("LHU	  R%d (0x%x) = Mem[R%d + 0x%x] \n",rt,rt_v,rs,s_ext_imm & 0xffff);
			break;                 
			case 0x30:	
				ex_out->ALU_res = rs_v + s_ext_imm;
				printf("LL	 R%d (0x%x) = Mem[R%d + 0x%x] \n",rt,rt_v,rs,s_ext_imm);
			break;
			case 0xf:
				ex_out->ALU_res = imm << 16;
				printf("LUI 	R%d (0x%x) = 0x%x \n",rt,ex_out->ALU_res,imm << 16);
			break;
			case 0x23:	
				ex_out->ALU_res = (int)rs_v + s_ext_imm;
				printf("LW	 R%d (0x%x) = Mem[R%d + 0x%x] \n",rt,rt_v,rs,s_ext_imm);
			break;
			case 0xd:
				ex_out->ALU_res = rs_v | z_ext_imm;  
				printf("ORI 	R%d (0x%x) = R%d | 0x%x \n",rt,ex_out->ALU_res,rs,z_ext_imm);
			break;
			case 0xa:
				ex_out->ALU_res = ((int)rs_v < s_ext_imm)? 1:0;
				printf("SLTI	R%d (0x%x) = (R%d < 0x%x)? 1:0 \n",rt,ex_out->ALU_res,rs,s_ext_imm);
			break;
			case 0xb:
				ex_out->ALU_res = (rs_v < (unsigned int)s_ext_imm)? 1:0; 
				printf("SLTIU		R%d (0x%x) = (R%d < 0x%x)? 1:0 \n",rt,ex_out->ALU_res,rs,s_ext_imm);
			break;
			case 0x28:
				ex_out->ALU_res = rs_v + s_ext_imm;
				printf("SB 	Mem[R%d + 0x%x] = R%d \n",rs,(s_ext_imm & 0xff),(reg[rt] & 0xff));
			break;
			case 0x29:
				ex_out->ALU_res = rs_v + s_ext_imm; 
				printf("SH	 Mem[R%d + 0x%x] = R%d \n",rs,s_ext_imm,(reg[rt] & 0xffff));
			break;
			case 0x2b:
				ex_out->ALU_res = rs_v + s_ext_imm;			
				printf("SW	 Mem[R%d + 0x%x] = %x \n",rs,s_ext_imm,rt_v);
			break;
			}					
		}
}


/*********************************************************************
load_store : load = mem에서 data 가져옴 / store = mem에 data 저장
 - count memory operation
**********************************************************************/

void load_store(EX_latch *ex_in, MEM_latch *mem_out, MEM_latch *mem_in){
	unsigned int rt_v, opcode, data;
	
	mem_out->inst = ex_in->inst;
	mem_out->valid = ex_in->valid;

	if(mem_out->valid){	
		mem_out->ALU_res = ex_in->ALU_res;
		mem_out->i = ex_in->i;
		mem_out->pc4 = ex_in->pc4;
		mem_out->rd = ex_in->rd;
		mem_out->data = ex_in->ALU_res;			
		mem_out->ALU_res = ex_in->ALU_res;
		rt_v = ex_in->rt_v;
		opcode = ex_in->i.opcode;

		if(ex_in->i.Mem){
			//count memory operation
			mem_op++;
			//load
			if(ex_in->i.MemRead){
				if(opcode == LL)
					mem_out->data = (int)Mem[(ex_in->ALU_res)/4];
				else if(opcode == LW)
					mem_out->data = Mem[(ex_in->ALU_res)/4];
				else if(opcode == LBU)
					mem_out->data = (Mem[ex_in->ALU_res]/4) & 0xff;
				else if(opcode == LHU)
					mem_out->data = (Mem[ex_in->ALU_res]/4) & 0xffff;

				printf("Mem : read(0x%x) from Mem[0x%x] \n",mem_out->data,ex_in->ALU_res);
			}
			//store
			else if(ex_in->i.MemWrite){
				if(opcode == SB)
					Mem[(mem_out->data)/4] = rt_v & 0xff;
				else if(opcode == SH)
					Mem[(mem_out->data)/4] = rt_v & 0xffff;
				else
					Mem[(mem_out->data)/4] = rt_v;
				printf("Mem : store(0x%x) to Mem[0x%x] \n",Mem[(ex_in->ALU_res)/4],ex_in->ALU_res);
			}
		}			
	}
}


/*************************************************************************
write_back : 결과값을 지정된 register에 저장
 - 프로그램 종료조건 : memory out latch의 inst=-1이면 종료
                       (마지막 inst WB까지 실행 된 이후 cycle에서 종료)
 - register[rd]에 data 저장(JAL, Load, 나머지 구별하여 data 분류)
**************************************************************************/

int write_back(MEM_latch *mem_in){
	unsigned int opcode = mem_in->i.opcode;
	unsigned int f_data;

	if(mem_in->inst == -1)
		return -1;

	if(mem_in->valid){
		if(mem_in->i.Rupdate){
			//count register operation
			reg_op++;

			if(opcode == JAL){
				f_data = mem_in->pc4 + 8;
			}
			else if(opcode == LBU || opcode == LHU || opcode == LL || opcode == LW ){
				f_data = mem_in->data;
			}
			else
				f_data = mem_in->ALU_res;
			
			reg[mem_in->rd] = f_data;
			printf("WB : R%d = 0x%x \n",mem_in->rd,f_data);
		}	
	}
	return 0;
}
/********************************************************
statistics : 프로그램 종료 후 결과 출력 
********************************************************/

void statistics(){
	printf("========= STATISTICS ========= \n");
	printf("# of cycle : %d \n",cycle);
	printf("# of mem ops : %d \n",mem_op);
	printf("# of reg ops : %d \n",reg_op);
	printf("# of (conditional) branches : %d \n",branch_op);
	printf("# of not-taken branches : %d \n",not_branch_op);
	printf("# of jumps : %d \n",jump_op);
	printf("RESULT : 0x%x \n",reg[2]);
}
