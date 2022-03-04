/* 
 * File:   stateMachine.h
 * Author: mikev
 *
 * Created on April 16, 2013, 10:27 AM
 */

#ifndef STATEMACHINE_H
#define	STATEMACHINE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "Transition.h"
#include "System.h"

//enum has to be given explicit initial value as it is used as array index
typedef enum {
    ST_IDLE=0,
    ST_MOTOR_RUN,	//1
	ST_BAG_INSERTED,	//2
	ST_ERROR_FLASH,	//3
} state;

//typedef int (*myFuncDef)(int, int); //function pointer with passed parameters and return value
typedef void (*action)(void);

typedef struct {
    state nextState;    // Enumerator for the next state
    action actionToDo;  // function-pointer to the action that shall be executed in current state
}  stateElement;        // structure for the elements in the state matrix


state StateMachine_stateEval(uint8_t event);
void StateMachine_init(void);
void StateMachine_idle(void);
void StateMachine_motorRun(void);
void StateMachine_bagInsertedTick(void);
void StateMachine_errorFlashTick(void);
void StateMachine_valveCycleDone(void);
void StateMachine_bagRemoved(void);
void StateMachine_insertedAgain(void);
void StateMachine_rxMsg(void);

#if DEBUG_RFID_TEST
void StateMachine_rfidTest(void);
#endif

uint8_t StateMachine_currentState;


#ifdef	__cplusplus
}
#endif

#endif	/* STATEMACHINE_H */
