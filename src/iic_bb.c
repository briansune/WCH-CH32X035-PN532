#include "iic_bb.h"

#define IIC_WD 1

void IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );

    IIC_SCL(1);
    IIC_SDA(1);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

}
//产生IIC起始信号
void IIC_Start(void)
{
    SDA_OUT();

    IIC_SDA(1);
    IIC_SCL(1);
    Delay_Us(IIC_WD);
    IIC_SDA(0);
    Delay_Us(IIC_WD);
    IIC_SCL(0);
    Delay_Us(IIC_WD*2);
}

void IIC_Stop(void)
{
    IIC_SCL(0);
    IIC_SDA(0);
    Delay_Us(IIC_WD);
    IIC_SCL(1);
    Delay_Us(IIC_WD);
    IIC_SDA(1);
    Delay_Us(IIC_WD);
}


u8 IIC_Wait_Ack(void)
{
    u8 ack=1;

    SDA_IN()
    IIC_SDA(1);
    Delay_Us(IIC_WD);
    IIC_SCL(1);
    Delay_Us(IIC_WD);

    if(!READ_SDA){
        ack=0;
    }
    IIC_SCL(0);
    Delay_Us(IIC_WD);
    SDA_OUT()

    return ack;
}

void IIC_Ack(void)
{
    IIC_SCL(0);
    IIC_SDA(0);
    Delay_Us(IIC_WD);
    IIC_SCL(1);
    Delay_Us(IIC_WD);
    IIC_SCL(0);
    Delay_Us(IIC_WD);
    IIC_SDA(1);
}

void IIC_NAck(void)
{
    IIC_SCL(0);
    IIC_SDA(1);
    Delay_Us(IIC_WD);
    IIC_SCL(1);
    Delay_Us(IIC_WD);
    IIC_SCL(0);
    Delay_Us(IIC_WD);
}

void IIC_Send_Byte(u8 txd)
{
    u8 t;

    for(t=0;t<8;t++)
    {
        IIC_SDA((txd&0x80)>>7);
        Delay_Us(IIC_WD);

        IIC_SCL(1);
        Delay_Us(IIC_WD);
        IIC_SCL(0);
        Delay_Us(IIC_WD);

        txd<<=1;
    }

    IIC_SCL(0);
    Delay_Us(IIC_WD);
}


u8 IIC_Read_Byte(void){

    unsigned char i,receive=0;

    SDA_IN();

    for(i=0;i<8;i++ )
    {
        IIC_SCL(1);
        Delay_Us(IIC_WD);
        receive<<=1;
        if(READ_SDA)
            receive|=0x01;

        IIC_SCL(0);
        Delay_Us(IIC_WD);
    }
    SDA_OUT();

    return receive;
}
