/* 
 * File:   EventStack.h
 * Author: mikev
 *
 * Created on April 18, 2013, 4:02 PM
 */

#ifndef EVENTSTACK_H
#define	EVENTSTACK_H

#include "System.h"

#ifdef	__cplusplus
extern "C" {
#endif
    
    void EventStack_push(int8_t event);
    int8_t EventStack_pop(void);
    void EventStack_init(void);

#ifdef	__cplusplus
}
#endif

#endif	/* EVENTSTACK_H */

