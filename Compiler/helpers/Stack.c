#include "Stack.h"
#include <stdio.h>
#include <assert.h>

void push(Stack* stack, int data)
{
	StackNode* ptr = calloc(1, sizeof(StackNode));
	ptr->prev = stack->top;
	stack->top = ptr;
}

int top(Stack* stack)
{
	assert(stack->top != NULL);
	return stack->top->data;
}

void pop(Stack* stack)
{
	assert(stack->top != NULL);
	void* new_top = stack->top->prev;
	free(stack->top);

	stack->top = new_top;
}
