#include "IRGenerator.h"
#include "logger.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Trie* localSymbolTable;
int localOffset;
int globalOffset = 0;

// helpers
int getNextLabel()
{
	static int label = 0;
	return ++label;
}

char* genNextVar()
{
	static int var = 0;
	char* name = calloc(10, sizeof(char));
	strcpy(name, "##temp");

	sprintf(name + strlen(name), "%d", var++);
	return name;
}

void fillOffsets(ASTNode* vars, FuncEntry* entry)
{
	while (vars)
	{
		VariableEntry* varEntry = vars->derived_type->structure;
		varEntry->offset = localOffset;
		varEntry->isGlobal = vars->isGlobal;
		localOffset += varEntry->isGlobal ? 0 : vars->derived_type->width;
		globalOffset += varEntry->isGlobal ? vars->derived_type->width : 0;

		vars = vars->sibling;
	}
}

// List operations
IRInsList* mergeLists(IRInsList* left, IRInsList* right)
{
	if (!left->head)
		return right;

	if (!right->head)
		return left;

	left->tail->next = right->head;
	left->tail = right->tail;
	return left;
}

IRInsList* insert(IRInsList* list, IRInstr* ins)
{
	IRInsNode* temp = calloc(1, sizeof(IRInsNode));
	temp->ins = ins;

	if (!list->head)
		list->head = list->tail = temp;
	else
		list->tail->next = temp, list->tail = temp;

	return list;
}

// main operations
void recurseiveGenFuncCode(ASTNode* stmt, Payload* payload)
{
	if (stmt == NULL)
		return;

	if (stmt->sym_index == 81)	// assignment
	{
		Payload to;
		to.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[0], &to);

		Payload exp;
		exp.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[1], &exp);

		IRInstr* ins = calloc(1, sizeof(IRInstr));
		ins->op = OP_ASSIGN;
		ins->dst.name = to.payload._arith.name;
		ins->src1.name = exp.payload._arith.name;

		payload->payload._stmt.code = exp.payload._arith.code;
		insert(payload->payload._stmt.code, ins);
	}
	else if (stmt->token->type == TK_ID)
	{
		payload->payload._arith.name = calloc(stmt->token->length + 1, sizeof(char));
		strcpy(payload->payload._arith.name, stmt->token->lexeme);
		payload->payload._arith.code = calloc(1, sizeof(IRInsList));
	}
	else if (stmt->token->type == TK_DOT)
	{
		Payload parent;
		parent.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[0], &parent);

		char* name = calloc(strlen(parent.payload._arith.name) + 1 + stmt->children[1]->token->length + 1, sizeof(char));
		strcpy(name, parent.payload._arith.name);
		free(parent.payload._arith.name);
		name[strlen(name)] = '.';
		strcpy(name + strlen(name), stmt->children[1]->token->lexeme);
		payload->payload._arith.name = name;
		payload->payload._arith.code = parent.payload._arith.code;
	}
	else if (stmt->sym_index == 86) // funcCall
	{
		// TODO
	}
	else if (stmt->sym_index == 89) // while -> B S
	{
		IRInstr* begin = calloc(1, sizeof(IRInstr));
		begin->op = OP_LABEL;
		begin->dst.int_val = getNextLabel();

		// B.true = label();
		// B.false = while.next
		Payload B;
		B.payload_type = PAYLOAD_BOOL;
		B.payload._bool._true.label_no = getNextLabel();
		B.payload._bool._false.label_no = payload->payload._stmt.label_next.label_no;
		recurseiveGenFuncCode(stmt->children[0], &B);

		// S.next = begin
		Payload S;
		S.payload_type = PAYLOAD_STMT;
		S.payload._stmt.label_next.label_no = begin->dst.int_val;
		recurseiveGenFuncCode(stmt->children[1], &S);

		// while.code = label(begin) || B.code || label(B.true) || S.code || goto(begin)

		IRInstr* btrue = calloc(1, sizeof(IRInstr));
		begin->op = OP_LABEL;
		begin->dst.int_val = B.payload._bool._true.label_no;

		IRInstr* gto = calloc(1, sizeof(IRInstr));
		begin->op = OP_JMP;
		begin->dst.int_val = begin->dst.int_val;

		payload->payload._stmt.code = calloc(1, sizeof(IRInsList));
		insert(payload->payload._stmt.code, begin);
		mergeLists(payload->payload._stmt.code, B.payload._bool.code);
		insert(payload->payload._stmt.code, btrue);
		mergeLists(payload->payload._stmt.code, S.payload._stmt.code);
		insert(payload->payload._stmt.code, gto);

		free(B.payload._bool.code);
		free(S.payload._stmt.code);
	}
	else if (stmt->sym_index == 90) // if -> B S
	{
		// B.true = label();
		// B.false = if.next
		Payload B;
		B.payload_type = PAYLOAD_BOOL;
		B.payload._bool._true.label_no = getNextLabel();
		B.payload._bool._false.label_no = payload->payload._stmt.label_next.label_no;
		recurseiveGenFuncCode(stmt->children[0], &B);

		// S.next = if.next
		Payload S;
		S.payload_type = PAYLOAD_STMT;
		S.payload._stmt.label_next.label_no = payload->payload._stmt.label_next.label_no;
		recurseiveGenFuncCode(stmt->children[1], &S);

		// if.code = B.code || label(B.true) || S.code
		IRInstr* instr = calloc(1, sizeof(IRInstr));
		instr->op = OP_LABEL;
		instr->dst.int_val = B.payload._bool._true.label_no;

		insert(B.payload._bool.code, instr);
		payload->payload._stmt.code = mergeLists(B.payload._bool.code, S.payload._stmt.code);

		// don't free B, it is pointed by if.payload
		free(S.payload._stmt.code);
	}
	else if (stmt->sym_index == 92)	// io
	{
		Payload to;
		to.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[0], &to);

		IRInstr* ins = calloc(1, sizeof(IRInstr));
		ins->op = stmt->token->type == TK_READ ? OP_READ : OP_WRITE;
		ins->dst.name = to.payload._arith.name;

		payload->payload._stmt.code = to.payload._arith.code;
	}
	else if (
		stmt->token->type == TK_LE ||
		stmt->token->type == TK_LT ||
		stmt->token->type == TK_GE ||
		stmt->token->type == TK_GT ||
		stmt->token->type == TK_EQ ||
		stmt->token->type == TK_NE)  // relop -> E1 E2
	{
		Payload E1;
		E1.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[0], &E1);

		Payload E2;
		E2.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[1], &E2);

		// relop.code = E1.code || E2.code || Operation || jump to relop.false
		IRInstr* code1 = calloc(1, sizeof(IRInstr));
		switch (stmt->token->type)
		{
		case TK_LE: code1->op = OP_LE; break;
		case TK_LT: code1->op = OP_LT; break;
		case TK_GE: code1->op = OP_GE; break;
		case TK_GT: code1->op = OP_GT; break;
		case TK_EQ: code1->op = OP_EQ; break;
		case TK_NE: code1->op = OP_NEQ; break;
		}

		code1->src1.name = E1.payload._arith.name;
		code1->src2.name = E2.payload._arith.name;
		code1->dst.int_val = payload->payload._bool._true.label_no;

		IRInstr* code2 = calloc(1, sizeof(IRInstr));
		code2->op = OP_JMP;
		code2->dst.int_val = payload->payload._bool._false.label_no;

		// merge
		insert(E2.payload._arith.code, code1);
		insert(E2.payload._arith.code, code2);
		payload->payload._bool.code = mergeLists(E1.payload._arith.code, E2.payload._arith.code);

		free(E2.payload._arith.code);
	}
	else if (stmt->token->type == TK_OR) // B -> B1 B2
	{
		Payload B1;
		B1.payload_type = PAYLOAD_BOOL;
		B1.payload._bool._true = payload->payload._bool._true;
		B1.payload._bool._false.label_no = getNextLabel();
		recurseiveGenFuncCode(stmt->children[0], &B1);

		Payload B2;
		B2.payload_type = PAYLOAD_BOOL;
		B2.payload._bool._true = payload->payload._bool._true;
		B2.payload._bool._false = payload->payload._bool._false;
		recurseiveGenFuncCode(stmt->children[1], &B2);

		// B.code = B1.code || B1.false || B2.code
		IRInstr* instr = calloc(1, sizeof(IRInstr));
		instr->op = OP_LABEL;
		instr->dst.int_val = B1.payload._bool._false.label_no;

		insert(B1.payload._bool.code, instr);
		payload->payload._bool.code = mergeLists(B1.payload._bool.code, B2.payload._bool.code);

		free(B2.payload._bool.code);
	}
	else if (stmt->token->type == TK_AND)
	{
		Payload B1;
		B1.payload_type = PAYLOAD_BOOL;
		B1.payload._bool._true.label_no = getNextLabel();
		B1.payload._bool._false = payload->payload._bool._false;
		recurseiveGenFuncCode(stmt->children[0], &B1);

		Payload B2;
		B2.payload_type = PAYLOAD_BOOL;
		B2.payload._bool._true = payload->payload._bool._true;
		B2.payload._bool._false = payload->payload._bool._false;
		recurseiveGenFuncCode(stmt->children[1], &B2);

		// B.code = B1.code || B1.true || B2.code
		IRInstr* instr = calloc(1, sizeof(IRInstr));
		instr->op = OP_LABEL;
		instr->dst.int_val = B1.payload._bool._true.label_no;

		insert(B1.payload._bool.code, instr);
		payload->payload._bool.code = mergeLists(B1.payload._bool.code, B2.payload._bool.code);

		free(B2.payload._bool.code);
	}
	else if (stmt->token->type == TK_NOT) // B -> !B1
	{
		Payload B1;
		B1.payload_type = PAYLOAD_BOOL;
		B1.payload._bool._true = payload->payload._bool._false;
		B1.payload._bool._false = payload->payload._bool._true;
		recurseiveGenFuncCode(stmt->children[0], &B1);

		payload->payload._bool.code = B1.payload._bool.code;
	}
	else if (
		stmt->token->type == TK_PLUS ||
		stmt->token->type == TK_MINUS ||
		stmt->token->type == TK_MUL ||
		stmt->token->type == TK_DIV
		)
	{
		payload->payload._arith.name = genNextVar();

		Payload E1;
		E1.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[0], &E1);

		Payload E2;
		E2.payload_type = PAYLOAD_ARITH;
		recurseiveGenFuncCode(stmt->children[0], &E2);

		IRInstr* ins = calloc(1, sizeof(IRInstr));

		switch (stmt->token->type)
		{
		case TK_PLUS: ins->op = OP_ADD; break;
		case TK_MINUS: ins->op = OP_SUB; break;
		case TK_MUL: ins->op = OP_MUL; break;
		case TK_DIV: ins->op = OP_DIV; break;
		}

		ins->src1.name = E1.payload._arith.name;
		ins->src2.name = E2.payload._arith.name;
		ins->dst.name = payload->payload._arith.name;

		mergeLists(E1.payload._arith.code, E2.payload._arith.code);
		insert(E1.payload._arith.code, ins);
		payload->payload._arith.code = E1.payload._arith.code;

		free(E2.payload._arith.code);
	}
	else if (stmt->token->type == TK_NUM)
	{
		// Store the value to temp
		IRInstr* store = calloc(1, sizeof(IRInstr));
		store->op = OP_STORE_INT;
		store->dst.name = genNextVar();
		store->src1.int_val = atoi(stmt->token->lexeme);

		payload->payload._arith.name = store->dst.name;
		payload->payload._arith.code = calloc(1, sizeof(IRInsList));
		insert(payload->payload._arith.code, store);
	}
	else if (stmt->token->type == TK_RNUM)
	{
		// Store the value to temp
		IRInstr* store = calloc(1, sizeof(IRInstr));
		store->op = OP_STORE_REAL;
		store->dst.name = genNextVar();
		store->src1.real_val = atof(stmt->token->lexeme);

		payload->payload._arith.name = store->dst.name;
		payload->payload._arith.code = calloc(1, sizeof(IRInsList));
		insert(payload->payload._arith.code, store);
	}

	// Goto siblings
	if (stmt->sibling)
	{
		assert(payload->payload_type == PAYLOAD_STMT);

		Payload p;
		p.payload_type = PAYLOAD_STMT;
		p.payload._stmt.label_next.label_no = getNextLabel();
		recurseiveGenFuncCode(stmt->sibling, &p);

		mergeLists(payload->payload._stmt.code, p.payload._stmt.code);
		free(p.payload._stmt.code);
	}
}

IRInsList* generateFuncCode(ASTNode* funcNode)
{
	// get symbol table
	TypeLog* mediator = trie_getRef(globalSymbolTable, funcNode->token->lexeme)->entry.ptr;
	FuncEntry* funcEntry = mediator->structure;
	localSymbolTable = funcEntry->symbolTable;
	localOffset = 0;

	logIt("Generating code for function: %s and its symbol symbol is found at adress: %p\n", funcEntry->name, localSymbolTable);

	// Iterate over statements
	fillOffsets(funcNode->children[1], funcEntry);
	fillOffsets(funcNode->children[0], funcEntry);
	fillOffsets(funcNode->children[2]->children[1], funcEntry);

	Payload p;
	p.payload_type = PAYLOAD_STMT;
	p.payload._stmt.label_next.label_no = getNextLabel();
	recurseiveGenFuncCode(funcNode->children[2]->children[2], &p);
	return p.payload._stmt.code;
}