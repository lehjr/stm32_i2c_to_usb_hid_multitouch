/*
 * i2c.c
 *
 *  Created on: 2018. 10. 17.
 *      Author: chand
 */
#include "stm32f1xx_hal.h"
#include "main.h"


#define i2cTime 3


enum{
	I2C_NOACK=0,
	I2C_ACK
};

#define SetSDA 	  TP_SDA_GPIO_Port->BSRR = TP_SDA_Pin
#define ResSDA    (TP_SDA_GPIO_Port->BSRR = (uint32_t)TP_SDA_Pin << 16U)
#define GetSDA    HAL_GPIO_ReadPin(TP_SDA_GPIO_Port, TP_SDA_Pin)
#define SetSCL    TP_SCL_GPIO_Port->BSRR = TP_SCL_Pin
#define ResSCL    (TP_SCL_GPIO_Port->BSRR = (uint32_t)TP_SCL_Pin << 16U)

uint8_t bNoAck=0;


void delay1us(uint32_t udelay)
{
  __IO uint32_t delayTick = (udelay * (SystemCoreClock / 10U / 1000000U));

  do
  {
    __NOP();
  }
  while(delayTick--);
}

void initI2C()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = TP_SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(TP_SDA_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = TP_SCL_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(TP_SCL_GPIO_Port, &GPIO_InitStruct);
	SetSCL;
	SetSDA;
 	HAL_Delay(100);
}

void i2c_start()
{
	SetSCL;
	delay1us(i2cTime);
	SetSDA;
	delay1us(i2cTime);
	ResSDA;
	delay1us(i2cTime);
}

void i2c_stop()
{
	ResSCL;
	delay1us(i2cTime);
	ResSDA;
	delay1us(i2cTime);
	SetSCL;
	delay1us(i2cTime);
	SetSDA;
	delay1us(i2cTime);
}

void Initial_I2C(void)
{
	i2c_stop();
}


uint8_t i2c_write(uint8_t dat)
{
	uint8_t ack;
	GPIO_InitTypeDef GPIO_InitStruct;
	uint8_t i;

	for(i = 0; i < 8; i++)
	{
		ResSCL;
		delay1us(i2cTime);

		if(dat & 0x80)
			SetSDA;
		else
			ResSDA;
		delay1us(i2cTime);

		dat <<=1;

		SetSCL;
		delay1us(i2cTime);
	}

	ResSCL;
	delay1us(i2cTime);
	SetSDA;
	delay1us(i2cTime);
	SetSCL;
	delay1us(i2cTime);

	GPIO_InitStruct.Pin = TP_SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(TP_SDA_GPIO_Port, &GPIO_InitStruct);

	if (GetSDA)
	{
		ack = I2C_NOACK;
	}
	else
	{
		ack = I2C_ACK;
	}

	GPIO_InitStruct.Pin = TP_SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(TP_SDA_GPIO_Port, &GPIO_InitStruct);


	return ack;
}


uint8_t i2c_read(uint8_t done)
{
	uint8_t i,dat=0;

	for(i = 0; i < 8; i++)
	{
		dat <<= 1;
		ResSCL;
		delay1us(i2cTime);
		SetSDA;
		delay1us(i2cTime);
		SetSCL;
		delay1us(i2cTime);

		if(GetSDA)
			dat |= 0x01;
	}

	ResSCL;
	delay1us(i2cTime);

	//ack
	if(done)
		SetSDA;
	else
		ResSDA;

	delay1us(i2cTime);

	SetSCL;
	delay1us(i2cTime);

	return dat;
}


uint8_t readI2C(uint8_t id,uint8_t reg, uint8_t *dat,uint16_t len)
{
	int i;
	uint8_t retryCount = 0;
	uint8_t ret=I2C_NOACK;
	do
	{
		retryCount++;

	// Stop
		i2c_stop();
		delay1us(i2cTime);

	// Start
		i2c_start();

	// Slave ID
		if(i2c_write(id) == I2C_NOACK)
		{
			continue;
		}

		// reg
		if(i2c_write(reg) == I2C_NOACK)
		{
			continue;
		}

		i2c_stop();
		delay1us(i2cTime);

		// Start
		i2c_start();

		if (i2c_write(id | 0x01) == I2C_NOACK)
		{
			continue;
		}

		// data
		for(i = 0; i < (len-1); i++)
		{
			dat[i] = i2c_read(0);
		}

		dat[i] = i2c_read(1);

		i2c_stop();
		ret = I2C_ACK;
	} while((retryCount < 10) || (ret == I2C_NOACK));
	delay1us(i2cTime);


	return ret;
}



uint8_t writeI2C(uint8_t id,uint8_t reg, uint8_t *dat, uint16_t len)
{
	int i;
	uint8_t retryCount = 0;
	uint8_t ret=0;
	do
	{
		ret = I2C_NOACK;
		retryCount++;

		i2c_stop();
		delay1us(i2cTime);

		i2c_start();

		// id
		if (i2c_write(id) == I2C_NOACK){
			continue;
		}

		// reg
		if (i2c_write(reg) == I2C_NOACK){
			continue;
		}
		ret = I2C_ACK;
		// data
		for(i = 0; i < len; i++)
		{
			if(i2c_write(dat[i]) == I2C_NOACK)
			{
				//if failed, ret = 0
				ret = I2C_NOACK;
				break;
			}
		}
		i2c_stop();
	}
	while((retryCount < 10) || (ret == I2C_NOACK));
	delay1us(i2cTime);

	return ret;
}

uint8_t read16I2C(uint8_t id, uint16_t reg, uint8_t *dat, uint16_t len)
{
	int i;
	uint8_t retryCount = 0;
	uint8_t ret=I2C_NOACK;
	do
	{
		retryCount++;

	// Stop
		i2c_stop();
		delay1us(i2cTime);

	// Start
		i2c_start();

	// Slave ID
		if(i2c_write(id) == I2C_NOACK)
		{
			continue;
		}

		// reg
		i2c_write(reg >> 8);
		if(i2c_write(reg & 0xFF) == I2C_NOACK)
		{
			continue;
		}

		i2c_stop();
		delay1us(i2cTime);

		// Start
		i2c_start();

		if (i2c_write(id | 0x01) == I2C_NOACK)
		{
			continue;
		}

		// data
		for(i = 0; i < (len-1); i++)
		{
			dat[i] = i2c_read(0);
		}

		dat[i] = i2c_read(1);

		i2c_stop();
		ret = I2C_ACK;
	} while((retryCount < 10) || (ret == I2C_NOACK));
	delay1us(i2cTime);


	return ret;
}

uint8_t write16I2C(uint8_t id, uint16_t reg, uint8_t *dat, uint16_t len)
{
	int i;
	uint8_t retryCount = 0;
	uint8_t ret=0;
	do
	{
		ret = I2C_NOACK;
		retryCount++;

		i2c_stop();
		delay1us(i2cTime);

		i2c_start();

		// id
		if (i2c_write(id) == I2C_NOACK){
			continue;
		}

		// reg
		i2c_write(reg >> 8);
		if(i2c_write(reg & 0xFF) == I2C_NOACK)
		{
			continue;
		}
		ret = I2C_ACK;
		// data
		for(i = 0; i < len; i++)
		{
			if(i2c_write(dat[i]) == I2C_NOACK)
			{
				//if failed, ret = 0
				ret = I2C_NOACK;
				break;
			}
		}
		i2c_stop();
	}
	while((retryCount < 10) || (ret == I2C_NOACK));
	delay1us(i2cTime);

	return ret;
}
