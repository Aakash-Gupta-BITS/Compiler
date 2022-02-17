#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Lexer.h"
#include "../helpers/ErrorHandling.h"
#include "../helpers/Globals.h"

char sourceCode[MAX_CODE_SIZE];
TokenNode* tokenList;

void loadData(char* loc, char** arr)
{
    FILE* fptr = fopen(loc, "r");

    if (fptr == NULL)
    {
        char BUFF[100];
        sprintf(BUFF, "Could not open file: %s", loc);
        displayError(BUFF);
        errorCode = FILE_READ_ERROR;
        return;
    }

    char BUFF[9];

    int lines_read = 0;
    while (fscanf(fptr, "%s", BUFF) != EOF)
    {
        lines_read++;
        if (lines_read == 1)
            continue;

        arr[lines_read - 2] = calloc(strlen(BUFF) + 1, sizeof(char));
        strcpy(arr[lines_read - 2], BUFF);
    }

    printf("File: %s\n", loc);
    for (int i = 1; i < lines_read; ++i)
        printf("\t%s\n", arr[i - 1]);

    printf("\n");

    fclose(fptr);
}

void loadCode(char* loc)
{
    FILE* fptr = fopen(loc, "r");

    if (fptr == NULL)
    {
        displayError("Could not open code file");
        errorCode = FILE_READ_ERROR;
        return;
    }

    char ch, * code_ptr = sourceCode;
    int count = 0;

    while ((ch = fgetc(fptr)) != EOF)
    {
        count++;
        if (count < 4)
            continue;

        *code_ptr = ch;
        code_ptr++;
    }
    *code_ptr = '\0';
}

TokenNode* DFA(int start_index)
{

}

TokenNode* symbolTable(int start_index)
{

}



/*
TokenNode* add(TokenNode* node, Token* token)
{
    if (node == NULL)
    {
        node = calloc(1, sizeof(TokenNode*));
        node->token = token;
        return node;
    }

    node->next = calloc(1, sizeof(TokenNode*));
    node->next->token = token;
    return node->next;
}

Token* selectAppropriateToken(Token* token1, Token* token2)
{
    if (token1 == token2)
        return token1;

    assert(token1 != NULL && token2 != NULL);

    if (token1->length > token2->length)
    {
        free(token2);
        return token1;
    }
    else if (token1->length < token2->length)
    {
        free(token1);
        return token2;
    }

    assert(token1->type != token2->type);

    if (token1->type < token2->type)
    {
        free(token2);
        return token1;
    }

    free(token1);
    return token2;
}

TokenNode* Process()
{
    TokenNode* head = NULL;

    TokenNode* tail = NULL;

    int line_number = 1;
    int start_index = 0;

    while (sourceCode[start_index] != '\0')
    {
        Token* cur_token = getNextToken(start_index);

        if (cur_token == NULL)
        {
            char msg[100];
            sprintf(msg, "There is a lexical error on line %d.", line_number);
            displayError(msg);
            errorCode = LEXICAL_ERROR;
            return NULL;
        }

        cur_token->line_number = line_number;
        start_index += cur_token->length;

        if (cur_token->type == WHITESPACE)
        {
            free(cur_token);

            if (sourceCode[start_index] == '\n')
                line_number++;

            continue;
        }

        tail = add(tail, cur_token);
        if (head == NULL)
            head = tail;
    }

    return head;
}*/