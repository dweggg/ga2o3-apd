/**
 * @file control.h
 * @author lan
 * @brief implement control task
 * @version 0.1
 * @date 2026-03-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef __CONTROL_H__
#define __CONTROL_H__

/**
 * @brief Initialize control related things
 * 
 * @return none
 */
void InitControl(void);

/**
 * @brief The main control task, better put it into an interrupt
 * 
 * @return none
 */
void TaskControl(void);


#endif
