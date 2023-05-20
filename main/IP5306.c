/*
 * IP5306.c
 *
 *  Created on: 9 May 2023
 *      Author: MertechArge014
 */


#include "IP5306.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "IP5306";

extern QueueHandle_t BatteryQueue;


/******************************************************************
*************************bit definitions***************************
*******************************************************************
*/

/*SYS_CTL0*/
union{
    struct{
     uint8_t BUTTON_SHUTDOWN             : 1;
     uint8_t SET_BOOST_OUTPUT_ENABLE     : 1;
     uint8_t POWER_ON_LOAD               : 1;
     uint8_t RSVD                        : 1;
     uint8_t CHARGER_ENABLE              : 1;
     uint8_t BOOST_ENABLE                : 1;
     uint8_t RSVD2                       : 2;

    }bits;

    uint8_t reg_byte;

}reg_SYS_CTL0_t;



/*SYS_CTL1*/
union{
    struct{
       uint8_t LOW_BATTERY_SHUTDOWN_ENABLE          : 1;
       uint8_t RSVD                                 : 1;
       uint8_t BOOST_AFTER_VIN                      : 1;
       uint8_t RSVD2                                : 2;
       uint8_t SHORT_PRESS_BOOST_SWITCH_ENABLE      : 1;
       uint8_t FLASHLIGHT_CTRL_SIGNAL_SELECTION     : 1;
       uint8_t BOOST_CTRL_SIGNAL_SELECTION          : 1;
    }bits;

    uint8_t reg_byte;

}reg_SYS_CTL1_t;


/*SYS_CTL2*/
union{
    struct{
       uint8_t RSVD                              : 2;
       uint8_t LIGHT_LOAD_SHUTDOWN_TIME          : 2;
       uint8_t LONG_PRESS_TIME                   : 1;
       uint8_t RSVD2                             : 3;
    }bits;

    uint8_t reg_byte;
}reg_SYS_CTL2_t;




/*Charger_CTL0*/
union{
     struct{
       uint8_t CHARGING_FULL_STOP_VOLTAGE      : 2;
       uint8_t RSVD                            : 6;
     }bits;

     uint8_t reg_byte;
}reg_Charger_CTL0_t;




/*Charger_CTL1*/
union{
    struct{
       uint8_t RSVD                             : 2;
       uint8_t CHARGE_UNDER_VOLTAGE_LOOP        : 3;
       uint8_t RSVD2                            : 1;
       uint8_t END_CHARGE_CURRENT_DETECTION     : 2;
    }bits;

    uint8_t reg_byte;
}reg_Charger_CTL1_t;




/*Charger_CTL2*/
union{
     struct{
       uint8_t  VOLTAGE_PRESSURE              : 2;
       uint8_t  BATTERY_VOLTAGE               : 2;
       uint8_t  RSVD                          : 4;
     }bits;

     uint8_t reg_byte;
}reg_Charger_CTL2_t;


/*Charger_CTL3*/
union{
     struct{
       uint8_t  RSVD                          : 5;
       uint8_t  CHARGE_CC_LOOP                : 1;
       uint8_t  RSVD2                         : 2;
     }bits;

     uint8_t reg_byte;
}reg_Charger_CTL3_t;


/*CHG_DIG_CTL0*/
union{
     struct{
       uint8_t  VIN_CURRENT                   : 5;
       uint8_t  RSVD                          : 3;
     }bits;

     uint8_t reg_byte;
}reg_CHG_DIG_CTL0_t;


/*REG_READ0*/
union{
      struct{
       uint8_t   RSVD                         : 3;
       uint8_t   CHARGE_ENABLE                : 1;
       uint8_t   RSVD2                        : 4;
      }bits;

      uint8_t  reg_byte;
}reg_READ0_t;

/*REG_READ1*/
union{
      struct{
       uint8_t   RSVD                          : 3;
       uint8_t   BATTERY_STATUS                : 1;
       uint8_t   RSVD2                         : 4;
      }bits;

      uint8_t  reg_byte;
}reg_READ1_t;


/*REG_READ2*/
union{
      struct{
       uint8_t   RSVD                         : 2;
       uint8_t   LOAD_LEVEL                   : 1;
       uint8_t   RSVD2                        : 5;
      }bits;

      uint8_t  reg_byte;
}reg_READ2_t;

/*REG_READ3*/
union{
      struct{
       uint8_t   SHORT_PRESS_DETECT           : 1;
       uint8_t   LONG_PRESS_DETECT            : 1;
       uint8_t   DOUBLE_PRESS_DETECT          : 1;
       uint8_t   RSVD                         : 5;
      }bits;

      uint8_t  reg_byte;
}reg_READ3_t;




static esp_err_t i2c_read(uint8_t ic_addr, uint8_t reg_addr)
{
	 uint8_t data;

     i2c_master_write_read_device(I2C_NUM_0, ic_addr, &reg_addr, 1, &data, 1, TIMEOUT_MS/portTICK_PERIOD_MS);
     return data;
}


static esp_err_t i2c_write(uint8_t ic_addr, uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(I2C_NUM_0, ic_addr, write_buf, sizeof(write_buf), TIMEOUT_MS/portTICK_PERIOD_MS);
    return ret;
}


void IP5306_Init()
{

	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = 21,
		.scl_io_num = 22,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 400000,
	};
	i2c_param_config(I2C_NUM_0, &conf);
	ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));


	  /*initialize register data*/
		reg_SYS_CTL0_t.reg_byte = i2c_read(IP5306_ADDRESS,SYS_CTL0);
		reg_SYS_CTL1_t.reg_byte = i2c_read(IP5306_ADDRESS,SYS_CTL1);
		reg_SYS_CTL2_t.reg_byte = i2c_read(IP5306_ADDRESS,SYS_CTL2);

		reg_Charger_CTL0_t.reg_byte = i2c_read(IP5306_ADDRESS,Charger_CTL0);
		reg_Charger_CTL1_t.reg_byte = i2c_read(IP5306_ADDRESS,Charger_CTL1);
	  reg_Charger_CTL2_t.reg_byte = i2c_read(IP5306_ADDRESS,Charger_CTL2);
	  reg_Charger_CTL3_t.reg_byte = i2c_read(IP5306_ADDRESS,Charger_CTL3);

}


/*@brief  select boost mode
  @param  enable or disable bit
  @retval None
  @note   Default value - enable
*/
void boost_mode(uint8_t boost_en){
	reg_SYS_CTL0_t.bits.BOOST_ENABLE = boost_en;

	i2c_write(IP5306_ADDRESS,SYS_CTL0,reg_SYS_CTL0_t.reg_byte);
}

/*@brief  select charger mode
  @param  enable or disable bit
  @retval None
  @note   Default value - enable
*/
void charger_mode(uint8_t charger_en){
	reg_SYS_CTL0_t.bits.CHARGER_ENABLE = charger_en;

	i2c_write(IP5306_ADDRESS,SYS_CTL0,reg_SYS_CTL0_t.reg_byte);
}

/*@brief  select auto power on once load detected
  @param  enable or disable bit
  @retval None
  @note   Default value - enable
*/
void power_on_load(uint8_t power_on_en){
	reg_SYS_CTL0_t.bits.POWER_ON_LOAD = power_on_en;

	i2c_write(IP5306_ADDRESS,SYS_CTL0,reg_SYS_CTL0_t.reg_byte);
}

/*@brief  boost o/p normally open function
  @param  enable or disable bit
  @retval None
  @note   Default value - enable
*/
void boost_output(uint8_t output_val){
	reg_SYS_CTL0_t.bits.SET_BOOST_OUTPUT_ENABLE = output_val;

	i2c_write(IP5306_ADDRESS,SYS_CTL0,reg_SYS_CTL0_t.reg_byte);
}

/*@brief  eneter shutdown mode using button
  @param  enable or disable bit
  @retval None
  @note   Default value - disable
*/
void button_shutdown(uint8_t shutdown_val){
	reg_SYS_CTL0_t.bits.BUTTON_SHUTDOWN = shutdown_val;

	i2c_write(IP5306_ADDRESS,SYS_CTL0,reg_SYS_CTL0_t.reg_byte);
}

/*@brief  boost control mode using button
  @param  enable or disable bit
  @retval None
  @note   Default value - disable
*/
void boost_ctrl_signal(uint8_t press_val){
	reg_SYS_CTL1_t.bits.BOOST_CTRL_SIGNAL_SELECTION = press_val;

	i2c_write(IP5306_ADDRESS,SYS_CTL1,reg_SYS_CTL1_t.reg_byte);
}

/*@brief  flashlight control mode using button
  @param  enable or disable bit
  @retval None
  @note   Default value - disable
*/
void flashlight_ctrl_signal(uint8_t press_val){
	reg_SYS_CTL1_t.bits.FLASHLIGHT_CTRL_SIGNAL_SELECTION = press_val;

	i2c_write(IP5306_ADDRESS,SYS_CTL1,reg_SYS_CTL1_t.reg_byte);
}

/*@brief
  @param  enable or disable bit
  @retval None
  @note   Default value - disable
*/
void short_press_boost(uint8_t boost_en){
	reg_SYS_CTL1_t.bits.SHORT_PRESS_BOOST_SWITCH_ENABLE = boost_en;

	i2c_write(IP5306_ADDRESS,SYS_CTL1,reg_SYS_CTL1_t.reg_byte);
}

/*@brief  keep boost mode on after input supply removal
  @param  enable or disable bit
  @retval None
  @note   Default value - enable
*/
void boost_after_vin(uint8_t val){
	reg_SYS_CTL1_t.bits.BOOST_AFTER_VIN = val;

	i2c_write(IP5306_ADDRESS,SYS_CTL1,reg_SYS_CTL1_t.reg_byte);
}

/*@brief  shutdown if battery voltage raches 3V
  @param  enable or disable bit
  @retval None
  @note   Default value - enable
*/
void low_battery_shutdown(uint8_t shutdown_en){
	reg_SYS_CTL1_t.bits.LOW_BATTERY_SHUTDOWN_ENABLE = shutdown_en;

	i2c_write(IP5306_ADDRESS,SYS_CTL1,reg_SYS_CTL1_t.reg_byte);
}

/*@brief  set long press timing
  @param  long press time value - 0 for 2s , 1 for 3s
  @retval None
  @note   Default value - disable
*/
void set_long_press_time(uint8_t press_time_val){
	reg_SYS_CTL2_t.bits.LONG_PRESS_TIME = press_time_val;

	i2c_write(IP5306_ADDRESS,SYS_CTL2,reg_SYS_CTL2_t.reg_byte);
}

/*@brief  set light load shutdown timing
  @param  shutdown time value - 0 for 8s, 1 for 32s, 2 for 16s and 3 for 64s
  @retval None
  @note   Default value - disable
*/
void set_light_load_shutdown_time(uint8_t shutdown_time){
  reg_SYS_CTL2_t.bits.LIGHT_LOAD_SHUTDOWN_TIME = shutdown_time;

  i2c_write(IP5306_ADDRESS,SYS_CTL2,reg_SYS_CTL2_t.reg_byte);
}

/*@brief  set charging cutoff voltage range for battery
  @param  voltage range value - 0 for 4.14/4.26/4.305/4.35  V
                                1 for 4.17/4.275/4.32/4.365 V
                                2 for 4.185/4.29/4.335/4.38 V
                                3 for 4.2/4.305/4.35/4.395  V
  @retval None
  @note   Default value - 2
*/
void set_charging_stop_voltage(uint8_t voltage_val){
	reg_Charger_CTL0_t.bits.CHARGING_FULL_STOP_VOLTAGE = voltage_val;

	i2c_write(IP5306_ADDRESS,Charger_CTL0,reg_Charger_CTL0_t.reg_byte);
}

/*@brief  set charging complete current detection
  @param  current value - 3 : 600mA
                          2 : 500mA
                          1 : 400mA
                          0 : 200mA

  @retval None
  @note   Default value - 1
*/
void end_charge_current(uint8_t current_val){
	reg_Charger_CTL1_t.bits.END_CHARGE_CURRENT_DETECTION = current_val;

	i2c_write(IP5306_ADDRESS,Charger_CTL1,reg_Charger_CTL1_t.reg_byte);
}

/*@brief  set voltage Vout for charging
  @param  voltage value - 111:4.8
                          110:4.75
                          101:4.7
                          100:4.65
                          011:4.6
                          010:4.55
                          001:4.5
                          000:4.45
  @retval None
  @note   Default value - 101
*/
void charger_under_voltage(uint8_t voltage_val){
	reg_Charger_CTL1_t.bits.CHARGE_UNDER_VOLTAGE_LOOP = voltage_val;

	i2c_write(IP5306_ADDRESS,Charger_CTL1,reg_Charger_CTL1_t.reg_byte);
}

/*@brief  set battery voltage
  @param  voltage value - 00:4.2
                          01:4.3
                          02:4.35
                          03:4.4
  @retval None
  @note   Default value - 00
*/
void set_battery_voltage(uint8_t voltage_val){
	reg_Charger_CTL2_t.bits.BATTERY_VOLTAGE = voltage_val;

	i2c_write(IP5306_ADDRESS,Charger_CTL2,reg_Charger_CTL2_t.reg_byte);
}

/*@brief  set constant voltage charging setting
  @param  voltage value - 11: Pressurized 42mV
                          10: Pressurized 28mV
                          01: Pressurized 14mV
                          00: Not pressurized
  @retval None
  @note   Default value - 01
  @note :4.30V/4.35V/4.4V It is recommended to pressurize 14mV
         4.2V It is recommended to pressurize 28mV
*/
void set_voltage_pressure(uint8_t voltage_val){
	reg_Charger_CTL2_t.bits.VOLTAGE_PRESSURE = voltage_val;

	i2c_write(IP5306_ADDRESS,Charger_CTL2,reg_Charger_CTL2_t.reg_byte);
}

/*@brief  set constant current charging setting
  @param  value - 1:VIN end CC Constant current
                  0:BAT end CC Constant current
  @retval None
  @note   Default value - 1
*/
void set_cc_loop(uint8_t current_val){
	reg_Charger_CTL3_t.bits.CHARGE_CC_LOOP = current_val;

	i2c_write(IP5306_ADDRESS,Charger_CTL3,reg_Charger_CTL3_t.reg_byte);
}

/*@brief  get current charging status
  @param  None
  @retval integer representation of charging status(1 - charging is on and 0 if  charging is off)
*/
uint8_t check_charging_status(void){
	reg_READ0_t.reg_byte = i2c_read(IP5306_ADDRESS,REG_READ0);

	return reg_READ0_t.bits.CHARGE_ENABLE;
}

/*@brief  get current battery status
  @param  None
  @retval integer representation of battery status(1 - fully charged and 0 if still charging)
*/
uint8_t check_battery_status(void){
	reg_READ1_t.reg_byte = i2c_read(IP5306_ADDRESS,REG_READ1);

	return reg_READ1_t.bits.BATTERY_STATUS;
}

/*@brief  get output load level
  @param  None
  @retval integer representation of load status(1 - light load and 0 if heavy load)
*/
uint8_t check_load_level(void){
	reg_READ2_t.reg_byte = i2c_read(IP5306_ADDRESS,REG_READ2);

	return reg_READ2_t.bits.LOAD_LEVEL;
}

/*@brief  detect if a short press occurred
  @param  None
  @retval button status
  @note clear this bit after reading by writing 1
*/
uint8_t short_press_detect(void){
	reg_READ3_t.reg_byte = i2c_read(IP5306_ADDRESS,REG_READ3);

	return reg_READ3_t.bits.SHORT_PRESS_DETECT;
}

/*@brief  detect if a long press occurred
  @param  None
  @retval button status
  @note clear this bit after reading by writing 1
*/
uint8_t long_press_detect(void){
	reg_READ3_t.reg_byte = i2c_read(IP5306_ADDRESS,REG_READ3);

	return reg_READ3_t.bits.LONG_PRESS_DETECT;
}

/*@brief  detect if a double press occurred
  @param  None
  @retval button status
  @note clear this bit after reading by writing 1
*/
uint8_t double_press_detect(void){
	reg_READ3_t.reg_byte = i2c_read(IP5306_ADDRESS,REG_READ3);

	return reg_READ3_t.bits.DOUBLE_PRESS_DETECT;
}


uint8_t Battery_Level(void)
        {
			uint8_t level;
			level = i2c_read(IP5306_ADDRESS,REG_READ4);

			ESP_LOGI(TAG,"Read REG_READ4 : %d \n", level);

			if (level & BATTERY_0_BIT)
				return 0;
			else if (level & BATTERY_25_BIT)
				return 25;
			else if (level & BATTERY_50_BIT)
				return 50;
			else if (level & BATTERY_75_BIT)
				return 75;

			return 100;
		}



void IP5306_Read_task()
{

	  uint8_t read_reg=0;
	  int batterylevel;


    //set battery voltage
     set_battery_voltage(BATT_VOLTAGE_0);   //4.2V

     //Voltage Vout for charging
     charger_under_voltage(VOUT_5);         //4.7V

     //set charging complete current
     end_charge_current(CURRENT_400);       //400mA

     //set cutoff voltage
     set_charging_stop_voltage(CUT_OFF_VOLTAGE_3);    // 4.2/4.305/4.35/4.395  V

     //set light load shutdown time
     set_light_load_shutdown_time(SHUTDOWN_64s);      //64s

     //enable low battery shutdown mode
     low_battery_shutdown(ENABLE);

     //allow boost even after removing Vin
     boost_after_vin(ENABLE);

     //allow auto power on after load detection
     power_on_load(ENABLE);

     //enable boost mode
     boost_mode(ENABLE);
	while (1) {


		static const char *IP_TASK_TAG = "IP5306_TASK";
		esp_log_level_set(IP_TASK_TAG, ESP_LOG_INFO);

		read_reg= i2c_read(IP5306_ADDRESS,SYS_CTL0);
		ESP_LOGI(IP_TASK_TAG,"Read SYS_CTL0 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,SYS_CTL1);
		ESP_LOGI(IP_TASK_TAG,"Read SYS_CTL1 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,SYS_CTL2);
		ESP_LOGI(IP_TASK_TAG,"Read SYS_CTL2 : 0x%02X \n", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,Charger_CTL0);
		ESP_LOGI(IP_TASK_TAG,"Read Charger_CTL0 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,Charger_CTL1);
		ESP_LOGI(IP_TASK_TAG,"Read Charger_CTL1 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,Charger_CTL2);
		ESP_LOGI(IP_TASK_TAG,"Read Charger_CTL2 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,Charger_CTL3);
		ESP_LOGI(IP_TASK_TAG,"Read Charger_CTL3 : 0x%02X \n", read_reg);


		read_reg= i2c_read(IP5306_ADDRESS,CHG_DIG_CTL0);
		ESP_LOGI(IP_TASK_TAG,"Read CHG_DIG_CTL0 : 0x%02X \n", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,REG_READ0);
		ESP_LOGI(IP_TASK_TAG,"Read REG_READ0 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,REG_READ1);
		ESP_LOGI(IP_TASK_TAG,"Read REG_READ1 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,REG_READ2);
		ESP_LOGI(IP_TASK_TAG,"Read REG_READ2 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,REG_READ3);
		ESP_LOGI(IP_TASK_TAG,"Read REG_READ3 : 0x%02X", read_reg);

		read_reg= i2c_read(IP5306_ADDRESS,REG_READ4);
		ESP_LOGI(IP_TASK_TAG,"Read REG_READ4 : 0x%02X \n", read_reg);



        batterylevel = (int) Battery_Level();
		ESP_LOGI(IP_TASK_TAG,"Battery Level : %d \n", batterylevel);

		 xQueueSend(BatteryQueue, &batterylevel, NULL);

		vTaskDelay(DELAY_MS/portTICK_PERIOD_MS);
	}
}


//==========================================================================================================
//
//==========================================================================================================
 void IP5306_test(void)
{
	 IP5306_Init();
  xTaskCreate(IP5306_Read_task, "IP5306_task", 1024*2, NULL, configMAX_PRIORITIES-2, NULL);
}
