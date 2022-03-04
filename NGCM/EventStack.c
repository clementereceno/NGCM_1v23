#include "EventStack.h"

int8_t stack[EVENT_STACK_DEPTH];
int8_t stackPtr;

/**
 * 
 * @param event
 */
void EventStack_push(int8_t event) {
    if(stackPtr < EVENT_STACK_DEPTH) {
        stack[++stackPtr] = event;  //push event onto stack
    }
}

int8_t EventStack_pop(void) {
    if(stackPtr >= 0) {
        return(stack[stackPtr--]);  //return stacked value
    } else {
        return(-1);                 //return empty stack indicator
    }
}

void EventStack_init(void) {
    stackPtr = -1;                  //stack is empty
}
