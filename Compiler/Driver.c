/***************************************
				GROUP-08
  Yash Bansal			-   2019A7PS0484P
  Sourabh S Yelluru		-   2018B3A70815P
  Nihir Agarwal			-   2018B4A70701P
  Aakash				-   2018B4A70887P
*****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"

#define MAX_OPTIONS 5

void clear_screen()
{
#ifdef _WIN32
	system("cls");
#endif
#ifdef _WIN64
	system("cls");
#endif
#ifdef __unix__
	system("clear");
#endif
#ifdef __linux__
	system("clear");
#endif
}

void printLexerOutput(char *path)
{
	FILE *fp = fopen(path, "r");

	if (!fp)
	{
		fprintf(stderr, "Error opening file %s: %s\n", path, strerror(errno));
		return;
	}

	loadFile(fp);

	Token *tk;
	while ((tk = getNextToken()) != NULL)
	{
		if (tk->type == TK_ERROR_LENGTH)
			printf("Line no. %d: Error: Identifier length is greater than the prescribed length.\n", tk->line_number);
		else if (tk->type == TK_ERROR_SYMBOL)
			printf("Line no. %d: Error: Unknwon Symbol <%s>\n", tk->line_number, tk->lexeme);
		else if (tk->type == TK_ERROR_PATTERN)
			printf("Line no. %d: Error: Unknown Pattern <%s>\n", tk->line_number, tk->lexeme);
		else
			printf("Line no. %d\tLexeme %s\t\tToken %s\n", tk->line_number, tk->lexeme, lexerData->tokenType2tokenStr[tk->type]);

		if (tk->lexeme != NULL)
			free(tk->lexeme);

		free(tk);
	}
}

void print_AST(ASTNode* ast, int tab)
{
	int temp = tab;
	while (temp--)
		printf("\t");


}

void main(int argc, char **argv)
{
	clear_screen();
	loadLexer();
	loadParser();

	if (argc != 3)
	{
		fprintf(stderr, "usage: stage1exe <source_ode_file> <parser_output_file>\n");
		exit(-1);
	}

	int option = 0;
	int start = 1;
	char c;

	clock_t start_time = clock();
	clock_t end_time = start_time;

	do
	{
		if (!start)
		{
			printf("Press any key to continue...");
			scanf("%c", &c);
			clear_screen();
		}
		start = 0;

		printf("Following are the options to select from\n");
		printf("0: Exit\n");
		printf("1: Remove Comments\n");
		printf("2: Print token list generated by lexer\n");
		printf("3: Parse the source code and print Parse Tree to file\n");
		printf("4: Print the time taken to verify syntactic correctness\n");
		printf("5: Compile\n");

		printf("Select an option: ");
		scanf("%d", &option);
		while ((c = getchar()) != '\n' && c != EOF)
			;
		clear_screen();

		if (option < 0 || option > MAX_OPTIONS)
		{
			printf("Invalid option selected: %d\n", option);
			continue;
		}

		if (option == 0)
		{
			freeLexerData();
			printf("Bye bye from group 8 compiler\n");
			break;
		}
		if (option == 4)
		{
			start_time = clock();
			TreeNode *node = parseInputSourceCode(argv[1]);
			end_time = clock();
			double total_CPU_time = (double)(end_time - start_time);
			printf("Total CPU Time Taken: %f\n", total_CPU_time);
			printf("Total CPU Time taken in seconds: %f\n", total_CPU_time / CLOCKS_PER_SEC);
			freeParseTree(node);
			continue;
		}

		if (option == 1)
		{
			FILE *source = fopen(argv[1], "r");

			if (source == NULL)
			{
				fprintf(stderr, "Error opening file %s: %s\n", argv[1], strerror(errno));
				return;
			}

			removeComments(source, stdout);

			fclose(source);
		}
		else if (option == 2)
			printLexerOutput(argv[1]);
		else if (option == 3)
		{
			TreeNode *node = parseInputSourceCode(argv[1]);

			FILE *fptr = fopen(argv[2], "w");
			fprintf(fptr, "Group 8 Output File.\n%30s %10s %30s %15s %30s %10s %30s\n",
					"Lexeme",
					"LineNumber",
					"TokenName",
					"Value (if Num)",
					"ParentSymbol",
					"Is Leaf",
					"NodeSymbol");

			printParseTree(node, fptr);
			fclose(fptr);

			freeParseTree(node);
		}
		else if (option == 5)
		{
			TreeNode *node = parseInputSourceCode(argv[1]);
			ASTNode *ast = createAST(node);
		}

	} while (option != 0);
}
