/** @file excp_handler.h
 *  @brief This file defines global variables and Macros used for 
 *         exception handling.
 */

#ifndef _EXCP_HANDLER_H
#define _EXCP_HANDLER_H

/** @brief The size of the exception stack in number of pages */
#define EXCP_STK_SIZE 1

/** @brief Get the value of specific bit */
#define BIT(d,n) (((d) >> (n)) & 1)

/** @brief The addr that one word higher than the exception stack */
void *esp3;

#endif /* _EXCP_HANDLER_H */
