/* 
 * File:   Transition.h
 * Author: mikev
 *
 * Created on April 25, 2013, 11:32 AM
 */

#ifndef TRANSITION_H
#define	TRANSITION_H

#ifdef	__cplusplus
extern "C" {
#endif
    void Transition_idleNormal(void);
    void Transition_motorRun(void);
    void Transition_bagInserted(void);
	void Transition_biToIdle(void);
	void Transition_idleMotorTimeout(void);
	void Transition_idleMotorOvld(void);
	void Transition_errorFlash(void);
 
#ifdef	__cplusplus
}
#endif

#endif	/* TRANSITION_H */

