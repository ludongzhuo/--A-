#include <stdint.h>
#include <stdbool.h>
#include "hw_memmap.h"
#include "debug.h"
#include "gpio.h"
#include "hw_i2c.h"
#include "hw_types.h"
#include "i2c.h"
#include "pin_map.h"
#include "sysctl.h"
#include "systick.h"
#include "interrupt.h"
//*****************************************************************************
//
//I2C GPIO chip address and resigster define
//
//*****************************************************************************
#define TCA6424_I2CADDR 					0x22
#define PCA9557_I2CADDR						0x18

#define PCA9557_INPUT							0x00
#define	PCA9557_OUTPUT						0x01
#define PCA9557_POLINVERT					0x02
#define PCA9557_CONFIG						0x03

#define TCA6424_CONFIG_PORT0			0x0c
#define TCA6424_CONFIG_PORT1			0x0d
#define TCA6424_CONFIG_PORT2			0x0e

#define TCA6424_INPUT_PORT0				0x00
#define TCA6424_INPUT_PORT1				0x01
#define TCA6424_INPUT_PORT2				0x02

#define TCA6424_OUTPUT_PORT0			0x04
#define TCA6424_OUTPUT_PORT1			0x05
#define TCA6424_OUTPUT_PORT2			0x06


void 		Delay(uint32_t value);
void 		S800_GPIO_Init(void);
uint8_t 	I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData);
uint8_t 	I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr);
void		S800_I2C0_Init(void);
volatile uint8_t result;
uint32_t ui32SysClock;
uint8_t seg7[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x58,0x5e,0x079,0x71,0x5c};
uint32_t value=0,count=0;
uint8_t pinlocation=0;
uint8_t systick_100ms_status=0,systick_10ms_status=0;
uint32_t systick_100ms_counter=100,systick_10ms_counter=10;
uint32_t flashtime[]={500,1000,2000,200};
uint8_t kk=0;
bool pressed;

int main(void)
{
	uint8_t k=0;
	//use internal 16M oscillator, HSI
  ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ |SYSCTL_OSC_INT |SYSCTL_USE_OSC), 16000000);		
	
	S800_GPIO_Init();
	S800_I2C0_Init();
	
	SysTickPeriodSet(ui32SysClock/1000);//1ms
	SysTickIntEnable(); 
	IntMasterEnable(); 
	SysTickEnable(); 
	
	while (1)
	{
		if(systick_10ms_status==1)
			{
				systick_10ms_status=0;
				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				if(k==1&&value==7)
				{
					result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[1]);
					result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,1);	
				}
				else
				{
					result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[value+1+k]);				//write port 1 	
		      result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,1<<(value+k));					//write port 2
					}
				k=1-k;
				if(value==7) pinlocation=0x81;
			  else pinlocation=(1<<value)|(1<<(value+1));
				result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,~pinlocation);	
			}
			
		if(systick_100ms_status==1)
		{
			systick_100ms_status=0;
			count++;
			if(count>=flashtime[kk]/100){
				count=0;
			  value++;
			  if(value==8) value=0;
			}
		}
	}
}

void Delay(uint32_t value)
{
	uint32_t ui32Loop;
	for(ui32Loop = 0; ui32Loop < value; ui32Loop++){};
}

void S800_GPIO_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);						//Enable PortF
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));			//Wait for the GPIO moduleF ready
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);						//Enable PortJ	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)){};		//Wait for the GPIO moduleJ ready	
	
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);			//Set PF0 as Output pin
	GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1);//Set the PJ0,PJ1 as input pin
	GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
}


void S800_I2C0_Init(void)
{
	uint8_t result;
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);//��ʼ��i2cģ��
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);//ʹ��I2Cģ��0����������ΪI2C0SCL--PB2��I2C0SDA--PB3
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);//����PB2ΪI2C0SCL
  GPIOPinConfigure(GPIO_PB3_I2C0SDA);//����PB3ΪI2C0SDA
  GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);//I2C��GPIO_PIN_2����SCL
  GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);//I2C��GPIO_PIN_3����SDA

	I2CMasterInitExpClk(I2C0_BASE,ui32SysClock, true);										//config I2C0 400k
	I2CMasterEnable(I2C0_BASE);	

	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT0,0x0ff);		//config port 0 as input
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT1,0x0);			//config port 1 as output
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT2,0x0);			//config port 2 as output 

	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_CONFIG,0x00);					//config port as output
	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x0ff);				//turn off the LED1-8
}


uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData)
{
	uint8_t rop;
	while(I2CMasterBusy(I2C0_BASE)){};//���I2C0ģ��æ���ȴ�
		//
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
		//��������Ҫ�ŵ������ϵĴӻ���ַ��false��ʾ����д�ӻ���true��ʾ�������ӻ�
		
	I2CMasterDataPut(I2C0_BASE, RegAddr);//����д�豸�Ĵ�����ַ
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);//ִ���ظ�д�����
	while(I2CMasterBusy(I2C0_BASE)){};
		
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);//������

	I2CMasterDataPut(I2C0_BASE, WriteData);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);//ִ���ظ�д�����������
	while(I2CMasterBusy(I2C0_BASE)){};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE);//������

	return rop;//���ش������ͣ��޴�����0
}

uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr)
{
	uint8_t value,rop;
	while(I2CMasterBusy(I2C0_BASE)){};	
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
//	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);		
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_SEND);//ִ�е���д�����
	while(I2CMasterBusBusy(I2C0_BASE));
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	Delay(1);
	//receive data
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true);//���ôӻ���ַ����
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_RECEIVE);//ִ�е��ζ�����
	while(I2CMasterBusBusy(I2C0_BASE));
	value=I2CMasterDataGet(I2C0_BASE);//��ȡ��ȡ������
		Delay(1);
	return value;
}

void SysTick_Handler(void)
{
	if(systick_100ms_counter!=0)
		systick_100ms_counter--;
	else 
	{
		systick_100ms_counter=100;
		systick_100ms_status=1;
	}
	
	if(systick_10ms_counter!=0)
		systick_10ms_counter--;
	else 
	{
		systick_10ms_counter=10;
		systick_10ms_status=1;
	}
	if(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)==0)
		pressed=true;
	else{
				if(pressed)
				{
					pressed=false;
					kk++;
					if(kk==4) kk=0;
				}
			}
}
