#ifndef __IIC_BB_H
#define __IIC_BB_H

#include "ch32x035.h"


//IO方向设置
#define SDA_IN()  {GPIOB->CFGHR&=0XFFFF0FFF;GPIOB->CFGHR|=8<<12;}
#define SDA_OUT() {GPIOB->CFGHR&=0XFFFF0FFF;GPIOB->CFGHR|=3<<12;}

//IO操作函数
#define IIC_SCL(i)       GPIO_WriteBit(GPIOB,GPIO_Pin_10,i) //SCL
#define IIC_SDA(i)       GPIO_WriteBit(GPIOB,GPIO_Pin_11,i)

#define READ_SDA         GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)

//IIC所有操作函数
void IIC_Init(void);                //初始化IIC的IO口
void IIC_Start(void);               //发送IIC开始信号
void IIC_Stop(void);                //发送IIC停止信号
void IIC_Send_Byte(u8 txd);         //IIC发送一个字节
u8 IIC_Read_Byte(void);//IIC读取一个字节
u8 IIC_Wait_Ack(void);              //IIC等待ACK信号
void IIC_Ack(void);                 //IIC发送ACK信号
void IIC_NAck(void);                //IIC不发送ACK信号

void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_Read_One_Byte(u8 daddr,u8 addr);
#endif
