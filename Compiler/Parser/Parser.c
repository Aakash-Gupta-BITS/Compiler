#include "Parser.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../helpers/Stack.h"
#include "../Lexer/Lexer.h"

/* for CHAR_BIT */
#define CHAR_BIT 8
#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

ParserData* parserData;


int isTerminal(int index)
{
	return index >= 0 && index < parserData->num_terminals;
}

char* bitAnd(char* bitset1, char* bitset2, int n, int* flag)
{
	char* bitset3 = calloc(n, sizeof(char));
	for (int i = 0; i < BITNSLOTS(n); i++) {
		bitset3[i] = bitset1[i] & bitset2[i];
		if (bitset3[i] != bitset1[i])
			*flag = 1;
	}
	return bitset3;
}
int isEqual(char* bitset1, char* bitset2, int n) {
	for (int i = 0; i < n; ++i)
		if (bitset1[i] != bitset2[i])
			return 0;
	return 1;
}
void setUnion(char* bitset1, char* bitset2, int n, int* flag) {

	for (int i = 0; i < n; i++) {
		char c = bitset1[i];
		
		bitset1[i] = bitset1[i] | bitset2[i];
		if (c != bitset1[i])  *flag = *flag + 1; 		

	}
}



void computeNullable()
{
	int n = parserData->num_non_terminals + parserData->num_terminals;
	int** rules = parserData->productions;

	char** productionBitset = calloc(parserData->num_productions, sizeof(char*));
	for (int i = 0; i < parserData->num_productions; i++) {
		productionBitset[i] = calloc(BITNSLOTS(n), sizeof(char));


		for (int j = 1; j < parserData->productionSize[i]; j++)
			BITSET(productionBitset[i], rules[i][j]);


	}

	parserData->nullable = calloc(BITNSLOTS(n), sizeof(char));

	for (int i = 0; i < parserData->num_productions; i++) {
		int rhsSize = parserData->productionSize[i] - 1;
		int lhs = rules[i][0];

		if (rhsSize == 1 && !(rules[i][1]))
			BITSET(parserData->nullable, rules[i][0]);


	}
	int flag = 1;
	while (flag) {
		flag = 0;
		for (int i = 0; i < parserData->num_productions; i++) {
			int changed = 0;
			if (isEqual(bitAnd(parserData->nullable, productionBitset[i], n, &changed), productionBitset[i], n)) {
				BITSET(parserData->nullable, rules[i][0]);
				flag = changed;
			}
		}
	}
	free(productionBitset);
}


void populateFirstSets()
{
	int n = parserData->num_non_terminals;
	int tnt = n + parserData->num_terminals;

	parserData->firstSet = calloc(n, sizeof(char*));
	for (int i = 0; i < n; i++)
		parserData->firstSet[i] = calloc(parserData->num_terminals, sizeof(char));
	int** rules = parserData->productions;
	int flag = 1;
	char* nullable = parserData->nullable;

	while (flag) {
		flag = 0;

		int change = 0;
		for (int i = 0; i < parserData->num_productions; i++) {
			int lhs = rules[i][0] - parserData->num_terminals;


			int k = parserData->productionSize[i];
			int nullableUntilNow = 1;
			int j = 1;

			while (j < k && nullableUntilNow) {
				if (isTerminal(rules[i][j])) {
					nullableUntilNow = 0;
					if (rules[i][j] == 0) {
					}
					if (!BITTEST(parserData->firstSet[lhs], rules[i][j])) {
						if (rules[i][j]) {
							BITSET(parserData->firstSet[lhs], rules[i][j]);

							change = 1;
						}
						else
							continue;
					}
					j++;
					continue;
				}
				setUnion(parserData->firstSet[lhs], parserData->firstSet[rules[i][j] - parserData->num_terminals], n, &change);
				if (!BITTEST(nullable, rules[i][j]))
					nullableUntilNow = 0;
				j++;
			}

		}
		flag = change;
	}
}

void populateFollowSets()
{
	int n = parserData->num_non_terminals;
	parserData->followSet = calloc(n, sizeof(char*));
	for (int i = 0; i < n; i++)
		parserData->followSet[i] = calloc(parserData->num_terminals, sizeof(char));
	int flag = 1;
	char* nullable = parserData->nullable;

	while (flag) {
		flag = 0;
		int change = 0;
		int ts = parserData->num_terminals;
		int nts = parserData->num_non_terminals;
		int** rules = parserData->productions;
		for (int ind = 0; ind < parserData->num_productions; ind++) {
			int k = parserData->productionSize[ind];
			int lhs = rules[ind][0] - ts;
			for (int i = 1; i < k; i++) {
				if (!isTerminal(rules[ind][i]))
				{
					char* temp = calloc(parserData->num_terminals, sizeof(char));
					int nullableFlag = 1;
					for (int j = i + 1; j < k; j++)
					{
						if (isTerminal(rules[ind][j]))
						{
							nullableFlag = 0;
							if (!BITTEST(temp, rules[ind][j]))
							{
								BITSET(temp, rules[ind][j]);
							}
							break;
						}
						else
						{
							int dummy = 0;
							setUnion(temp, parserData->firstSet[rules[ind][j] - ts], nts, &dummy);
							if (!BITTEST(nullable, rules[ind][j]))
							{
								nullableFlag = 0;
								break;
							}
						}

					}
					setUnion(parserData->followSet[rules[ind][i] - ts], temp, nts, &change);
					if (nullableFlag || i == k - 1)
					{
						setUnion(parserData->followSet[rules[ind][i] - ts], parserData->followSet[lhs], nts, &change);
					}

				}
			}
		}
		flag = change;
	}
}

void computeFirstFollowSets()
{
	computeNullable();
	populateFirstSets();
	populateFollowSets();
}


void computeParseTable()
{
	parserData->parseTable = calloc(parserData->num_non_terminals, sizeof(int*));

	for (int i = 0; i < parserData->num_non_terminals; i++)
		parserData->parseTable[i] = calloc(parserData->num_terminals, sizeof(int));

	for (int i = 0; i < parserData->num_non_terminals; i++)
		for (int j = 0; j < parserData->num_terminals; j++)
			parserData->parseTable[i][j] = -1;

	int** rules = parserData->productions;
	char* nullable = parserData->nullable;

	for (int ind = 0; ind < parserData->num_productions; ind++)
	{
		int lhs = rules[ind][0] - parserData->num_terminals;
		int k = parserData->productionSize[ind];

		char* temp = calloc(parserData->num_terminals, sizeof(char));
		for (int j = 1; j < k; j++)
		{
			if (isTerminal(rules[ind][j]))
			{
				if (!BITTEST(temp, rules[ind][j]))
					BITSET(temp, rules[ind][j]);
				break;
			}

			int dummy = 0;
			setUnion(temp, parserData->firstSet[rules[ind][j] - parserData->num_terminals], parserData->num_non_terminals, &dummy);

			if (!BITTEST(nullable, rules[ind][j]))
				break;
		}
		
		if (!(rules[ind][1] == 0))
			for (int i = 0; i < parserData->num_terminals; i++)
				if (BITTEST(temp, i))
					parserData->parseTable[lhs][i] = ind;
		
		char* followSet = parserData->followSet[lhs];
		int ruleIsNullable = 1;
		for (int i = 0; i < parserData->num_terminals; i++)
			if (BITTEST(temp, i) && !BITTEST(nullable, i)) 
				ruleIsNullable = 0;
		
		if (ruleIsNullable || (rules[ind][1] == 0 && BITTEST(nullable, lhs + parserData->num_terminals)))
			for (int i = 0; i < parserData->num_terminals; i++)
				if (BITTEST(followSet, i))
					parserData->parseTable[lhs][i] = ind;

		free(temp);
	}

	for (int i = 0; i < parserData->num_non_terminals; i++)
		for (int j = 0; j < parserData->num_terminals; j++)
			if (parserData->parseTable[i][j] == -1 && BITTEST(parserData->followSet[i], j))
				parserData->parseTable[i][j] = -2;

	for (int i = 0; i < parserData->num_terminals; ++i)
	{
		for (int j = 0; j < parserData->num_non_terminals; ++j)
			printf("%3d ", parserData->parseTable[j][i]);
		printf("\n");
	}
}

void loadSymbols(FILE* fp)
{
	for (int i = 0; i < parserData->num_terminals + parserData->num_non_terminals; ++i)
    {
        char BUFF[64];
        fscanf(fp, "%s\n", BUFF);
        parserData->symbolType2symbolStr[i] = calloc(strlen(BUFF) + 1, sizeof(char));
        strcpy(parserData->symbolType2symbolStr[i], BUFF);
        TrieNode* ref = trie_getRef(parserData->symbolStr2symbolType, BUFF);
        ref->entry.value = i;
    }
}

void loadProductions(FILE* fp)
{
	parserData->productions = calloc(parserData->num_productions, sizeof(int*));
	parserData->productionSize = calloc(parserData->num_productions, sizeof(int));
	for (int i = 0; i < parserData->num_productions; i++)
	{
		char BUFF[200];
		int symbols[200];
		fgets(BUFF, 200, fp);
		char* token = strtok(BUFF, " ");
		int count = 0;
		while (token)
		{
			if (strcmp(token, "\n") == 0) {}
			else
			{
				symbols[count] = trie_getVal(parserData->symbolStr2symbolType, token).value;
				count++;
			}
			token = strtok(NULL, " ");
		}
		parserData->productions[i] = calloc(count, sizeof(int));
		parserData->productionSize[i] = count;
		for (int j = 0; j < count; j++)
			parserData->productions[i][j] = symbols[j];

	}
	computeFirstFollowSets();
	computeParseTable();
}

void loadParser()
{
	assert(parserData == NULL);

	FILE* fp = fopen("./Parser/Parser_Structure.txt", "r");
	parserData = calloc(1, sizeof(ParserData));

	assert(fp != NULL);

	fscanf(fp, "%d %d %d %d\n", &parserData->num_terminals, &parserData->num_non_terminals, &parserData->num_productions, &parserData->start_index);
	parserData->symbolType2symbolStr = calloc(parserData->num_terminals + parserData->num_non_terminals, sizeof(char*));
	parserData->symbolStr2symbolType = calloc(1, sizeof(Trie));
	loadSymbols(fp);
	loadProductions(fp);
	fclose(fp);
}

int lexerToParserToken(int index)
{
	return trie_getVal(parserData->symbolStr2symbolType, lexerData->tokenType2tokenStr[index]).value;
}

TreeNode* recoverFromError(TreeNode* node, Stack* st, Token** lookahead)
{
	if (isTerminal(top(st)))	// terminal unmatch with terminal
	{
		TreeNode* parent = node->parent;
		for (int i = node->parent_child_index; i < parent->child_count; ++i)
		{
			pop(st);
			free(parent->children[i]);
		}

		if (node->parent_child_index == 0)
			free(parent->children);

		node = parent;

		// update lookahead
		assert(!isTerminal(node->symbol_index));
	}
	else
		pop(st);

	// when we have non terminal on top of stack
	char* followSet = parserData->followSet[node->symbol_index];
	while (
		(*lookahead)->type == TK_ERROR_LENGTH ||
		(*lookahead)->type == TK_ERROR_PATTERN ||
		(*lookahead)->type == TK_ERROR_SYMBOL ||
		BITTEST(followSet, lexerToParserToken((*lookahead)->type)))
	{
		free((*lookahead)->lexeme);
		free(*lookahead);
		*lookahead = getNextToken();
	}

	while (node->parent_child_index == node->parent->child_count - 1)
		node = node->parent;
	return node = node->parent->children[node->parent_child_index + 1];
}

TreeNode* parseSourceCode(char* fileLoc)
{
	TreeNode* parseTree;
	FILE* fp = fopen(fileLoc, "r");
	loadFile(fp);

	Stack* s = calloc(1, sizeof(Stack));
	s->top = NULL;

	push(s, -1);
	push(s, parserData->start_index);

	parseTree = calloc(1, sizeof(TreeNode));
	parseTree->parent = NULL;
	parseTree->symbol_index = top(s);

	int current_top = top(s);
	TreeNode* node = parseTree;

	Token* lookahead = getNextToken();
	parseTree->parent = NULL;
	while (lookahead != NULL)
	{
		if (!isTerminal(current_top))
		{
			int row = current_top - parserData->num_terminals;
			int column = lexerToParserToken(lookahead->type);

			int production_index = parserData->parseTable[row][column];

			if (production_index < 0)
			{
				node = recoverFromError(node, s, &lookahead);
				continue;
			}

			int* production = parserData->productions[production_index];
			int production_size = parserData->productionSize[production_index];

			pop(s);

			if (production_size == 2 && production[1] == 0)
			{
				current_top = top(s);

				while (node->parent_child_index == node->parent->child_count - 1)
					node = node->parent;
				node = node->parent->children[node->parent_child_index + 1];
				
				continue;
			}

			node->child_count = production_size - 1;
			node->children = calloc(node->child_count, sizeof(TreeNode*));

			for (int i = production_size - 1; i > 0; --i)
			{
				push(s, production[i]);

				// -1 here because production[0] is the start symbol
				node->children[i - 1] = calloc(1, sizeof(TreeNode));

				node->children[i - 1]->parent = node;
				node->children[i - 1]->symbol_index = production[i];
				node->children[i - 1]->parent_child_index = i - 1;
				node->children[i - 1]->isLeaf = 0;
			}

			current_top = top(s);
			node = node->children[0];
			continue;
		}

		// terminal

		if (lookahead->type == TK_ERROR_LENGTH || 
			lookahead->type == TK_ERROR_PATTERN || 
			lookahead->type == TK_ERROR_SYMBOL ||
			current_top != lexerToParserToken(lookahead->type))
		{
			node = recoverFromError(node, s, &lookahead);
			continue;
		}
			

		assert(current_top == lexerToParserToken(lookahead->type));
		node->token = lookahead;
		node->isLeaf = 1;
		lookahead = getNextToken();

		pop(s);
		current_top = top(s);
		if (top(s) == -1)
			break;

		while (node->parent_child_index == node->parent->child_count - 1)
			node = node->parent;
		node = node->parent->children[node->parent_child_index + 1];
		
		assert(current_top == node->symbol_index);
	}

	assert(current_top == -1);

	return parseTree;
}
