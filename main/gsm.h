/*
 * gsm.h
 *
 *  Created on: 9 May 2023
 *      Author: MertechArge014
 */

#ifndef MAIN_GSM_H_
#define MAIN_GSM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define MODEM_RST             32
#define MODEM_PWRKEY         32
#define MODEM_POWER_ON       32

#define MODEM_TX             33
#define MODEM_RX             35

#define MODEM_DTR            12
#define MODEM_RI             37


#define HI_LEVEL             1
#define LO_LEVEL             0




 void GSM_Init(void);


#endif /* MAIN_GSM_H_ */
