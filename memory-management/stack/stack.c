#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct {

        int top; // the top of the stack
        unsigned capacity; // the capacity should be positive
        int* array; // the array of the stack

} Stack;



Stack* createstack(unsigned capacity)
{

        Stack* stack = (Stack*)malloc(sizeof(Stack));
        stack->capacity = capacity;
        stack->top = -1;
        stack->array = (int*)malloc(stack->capacity * sizeof(int));
        return stack;
}

int isfull(Stack* stack)
{
     return stack->top == stack->capacity -1;

}

int isempty(Stack* stack)
{
  return stack->top == -1;

}

void pushstack(Stack* stack, int item)
{
        if(isfull(stack)){
                printf("Stack overflow");
        }
        else{stack->array[++stack->top] = item;
                printf("%d pushed to stack\n", item);}
}
int popstack(Stack* stack)
{
        if(isempty(stack)){
                return INT_MIN;
        }
        return stack->array[--stack->top];

}

int main()
{
        Stack* stack = createstack(50);
        pushstack(stack, 10);
        pushstack(stack, 20);
        pushstack(stack, 40);

        printf("%d popped from stack\n", popstack(stack));

        return 0;
}

