/*
 * utils.c
 *
 *  Created on: 2012-4-25
 *      Author: Benn Huang (benn@allwinnertech.com)
 */

extern void wait(int cycle)
{
	volatile int i = 0;

	while (i<=cycle) {
		i++;
	}
}
