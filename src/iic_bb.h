#ifndef __IIC_BB_H
#define __IIC_BB_H

#include "ch32x035.h"


//IO��������
#define SDA_IN()  {GPIOB->CFGHR&=0XFFFF0FFF;GPIOB->CFGHR|=8<<12;}
#define SDA_OUT() {GPIOB->CFGHR&=0XFFFF0FFF;GPIOB->CFGHR|=3<<12;}

//IO��������
#define IIC_SCL(i)       GPIO_WriteBit(GPIOB,GPIO_Pin_10,i) //SCL
#define IIC_SDA(i)       GPIO_WriteBit(GPIOB,GPIO_Pin_11,i)

#define READ_SDA         GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)

//IIC���в�������
void IIC_Init(void);                //��ʼ��IIC��IO��
void IIC_Start(void);               //����IIC��ʼ�ź�
void IIC_Stop(void);                //����IICֹͣ�ź�
void IIC_Send_Byte(u8 txd);         //IIC����һ���ֽ�
u8 IIC_Read_Byte(void);//IIC��ȡһ���ֽ�
u8 IIC_Wait_Ack(void);              //IIC�ȴ�ACK�ź�
void IIC_Ack(void);                 //IIC����ACK�ź�
void IIC_NAck(void);                //IIC������ACK�ź�

void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_Read_One_Byte(u8 daddr,u8 addr);
#endif
