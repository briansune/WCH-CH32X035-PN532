#include "nfc.h"

u8 hextab[17]="0123456789ABCDEF";
u8 ack[6]={
    0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00
};
u8 nfc_version[6]={
    0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03
};

/** data buffer */
u8 nfc_buf[NFC_CMD_BUF_LEN];

u8 send(u8 data){

    IIC_Send_Byte(data);
    return IIC_Wait_Ack();
}

u8 send_hold(u8 data){

    u8 flag = IIC_Send_Byte_Hold(data);

    if(flag){
        return flag;
    }

    return IIC_Wait_Ack();
}

void nfc_init(void){
    IIC_Init();
}

u8 write_cmd(u8 *cmd, u8 len){

    u8 checksum = 0;
    u8 ack;
    checksum += PN532_HOSTTOPN532;

    len++;

redo_again:
    IIC_Start();
    IIC_Send_Byte(PN532_I2C_ADDRESS);
    ack = IIC_Wait_Ack();
    if(ack){
        IIC_Stop();
        goto redo_again;
    }

    ack = send_hold(PN532_PREAMBLE);
    if(ack){
        goto redo_again;
    }

    send(PN532_PREAMBLE);
    send(PN532_STARTCODE2);
    send(len);
    send(-len);

    send(PN532_HOSTTOPN532);

    for (uint8_t i=0; i<len-1; i++){
        send(cmd[i]);
        checksum += cmd[i];
    }

    send(-checksum);
    send(PN532_POSTAMBLE);

    IIC_Stop();
    return 1;
}

void read_dt(u8 *buf, u8 len){

    buf[0] = 0x00;

    while(buf[0] != 0x01){
        IIC_Start();
        IIC_Send_Byte(PN532_I2C_ADDRESS + 1);
        if(IIC_Wait_Ack()){
            IIC_Stop();
            continue;
        }

        buf[0] = IIC_Read_Byte();
        IIC_Ack();

        if(buf[0] != 0x01){
            IIC_Stop();
        }else
            break;
    }

    for (u8 i=0; i<len; i++)
    {
        buf[i] = IIC_Read_Byte();

        if(i==len-1)
            IIC_NAck();
        else {
            IIC_Ack();
        }

        if((len!=6)&&i==3&&(buf[0]==0)&&(buf[1]==0)&&(buf[2]==0xFF)){
            len = buf[i]+6;
        }

    }

    IIC_Stop();
}

u8 read_ack(void){

    u8 ack_buf[6];

    read_dt(ack_buf, 6);

    return (0 == strncmp((char *)ack_buf, (char *)ack, 6));
}

u8 write_cmd_check_ack(u8 *cmd, u8 len)
{
    write_cmd(cmd, len);

    if (!read_ack()) {
        return 0;
    }

    return 1;
}

u32 get_version(void){

    u32 version;

    nfc_buf[0] = PN532_COMMAND_GETFIRMWAREVERSION;
    if(!write_cmd_check_ack(nfc_buf, 1)){
        return 0;
    }
//    wait_ready(NFC_WAIT_TIME);
    read_dt(nfc_buf, 12);
    if(nfc_buf[5] != 0xD5){
        return 0;
    }
    // check some basic stuff
    if (0 != strncmp((char *)nfc_buf, (char *)nfc_version, 6)) {
        return 0;
    }

    version = nfc_buf[7];
    version <<= 8;
    version |= nfc_buf[8];
    version <<= 8;
    version |= nfc_buf[9];
    version <<= 8;
    version |= nfc_buf[10];

    return version;
}

u8 CiuRfConfiguration(u8 use_amp, u8 amp_gain, u8 rf_level){

    nfc_buf[0] = PN532_COMMAND_WRITEREGISTER;
    nfc_buf[1] = 0x63;
    nfc_buf[2] = 0x16;
    nfc_buf[3] = ((use_amp & 0x01) << 8) | ((amp_gain & 0x07) << 4) | (rf_level & 0x0F);

    if(!write_cmd_check_ack(nfc_buf, 4)){
        return 0;
    }

    // read data packet
    read_dt(nfc_buf, 8);

    return  (nfc_buf[6] == 0x09);
}

u8 PN_reset(void){

    nfc_buf[0] = PN532_COMMAND_WRITEREGISTER;
    nfc_buf[1] = 0x62;
    nfc_buf[2] = 0x03;
    nfc_buf[3] = 0xC1;

    if(!write_cmd_check_ack(nfc_buf, 4)){
        return 0;
    }

    // read data packet
    read_dt(nfc_buf, 8);

    return  (nfc_buf[6] == 0x09);
}


void puthex(u8 *buf, u32 len){

    u32 i;
    for(i=0; i<len; i++)
    {
        printf("%C", hextab[(buf[i]>>4)&0x0F]);
        printf("%C", hextab[buf[i]&0x0F]);
        printf(" ");
    }
}

u8 InListPassiveTarget(u8 *buf, u8 brty, u8 maxtg, u8 *idata){

    nfc_buf[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    nfc_buf[1] = maxtg;
    nfc_buf[2] = brty;

    if(!write_cmd_check_ack(nfc_buf, 3)){
        return 0;
    }

    /** "Waiting for IRQ (indicates card presence)" */
//    wait_ready(NFC_WAIT_TIME);

    read_dt(nfc_buf,20);
    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_INLISTPASSIVETARGET+1)){
        return 0;
    }
//    printf(" Found Card.\n");

    if(nfc_buf[NFC_FRAME_ID_INDEX+1]!=1){
        printf("%d", nfc_buf[NFC_FRAME_ID_INDEX+1]);
        return 0;
    }
    /** UUID length */
    buf[0] = nfc_buf[12];

    for(u8 i=1; i<5; i++){
        buf[i] = nfc_buf[12+i];
    }
    return 1;
}

u8 MifareReadBlock(u8 block, u8 *buf){

    nfc_buf[0] = PN532_COMMAND_INDATAEXCHANGE;
    nfc_buf[1] = 1; // logical number of the relevant target
    nfc_buf[2] = MIFARE_CMD_READ;
    nfc_buf[3] = block;

    if(!write_cmd_check_ack(nfc_buf, 4)){
        return 0;
    }
//    wait_ready(NFC_WAIT_TIME);
    read_dt(nfc_buf, 26);

    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_INDATAEXCHANGE+1)){
        return 0;
    }
    if(nfc_buf[NFC_FRAME_ID_INDEX+1]){
        return 0;
    }

    memcpy(buf, nfc_buf+8, 16);

    return 1;
}

u8 MifareAuthentication(
        u8 type, u8 block, u8 *uuid, u8 uuid_len, u8 *key
){

    u8 i;
    nfc_buf[0] = PN532_COMMAND_INDATAEXCHANGE;
    nfc_buf[1] = 1; // logical number of the relevant target
    nfc_buf[2] = MIFARE_CMD_AUTH_A+type;
    nfc_buf[3] = block;

    for(i=0; i<6; i++){
        nfc_buf[4+i] = key[i];
    }
    for(i=0; i<uuid_len; i++){
        nfc_buf[10+i] = uuid[i];
    }

    if(!write_cmd_check_ack( nfc_buf, (10+uuid_len) )){
        return 0;
    }

//    wait_ready(NFC_WAIT_TIME);
    read_dt(nfc_buf, 8);
    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_INDATAEXCHANGE+1)){
        return 0;
    }
    if(nfc_buf[NFC_FRAME_ID_INDEX+1]){
        return 0;
    }
    return 1;
}


u8 SAMConfiguration(u8 mode, u8 timeout, u8 irq){

    nfc_buf[0] = PN532_COMMAND_SAMCONFIGURATION;
    nfc_buf[1] = mode; // normal mode;
    nfc_buf[2] = timeout; // timeout 50ms * 20 = 1 second
    nfc_buf[3] = irq; // use IRQ pin!

    if(!write_cmd_check_ack(nfc_buf, 4)){
        return 0;
    }

    // read data packet
    read_dt(nfc_buf, 8);

    return  (nfc_buf[6] == PN532_COMMAND_SAMCONFIGURATION);
}

u8 P2PInitiatorInit(void){

    /** avoid resend command */
    static u8 send_flag=1;
    nfc_buf[0] = PN532_COMMAND_INJUMPFORDEP;
    nfc_buf[1] = 0x01; // avtive mode
    nfc_buf[2] = 0x02; // 424 Kbps

    nfc_buf[3] = 0x01;

    nfc_buf[4] = 0x00;
    nfc_buf[5] = 0xFF;
    nfc_buf[6] = 0xFF;
    nfc_buf[7] = 0x00;
    nfc_buf[8] = 0x00;

    if(send_flag){
        send_flag = 0;
        if(!write_cmd_check_ack(nfc_buf, 9)){
            return 0;
        }
    }

    read_dt(nfc_buf, 25);

    if(nfc_buf[5] != 0xD5){
        return 0;
    }

    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_INJUMPFORDEP+1)){
        return 0;
    }
    if(nfc_buf[NFC_FRAME_ID_INDEX+1]){
        return 0;
    }

    send_flag = 1;
    return 1;
}

u8 P2PInitiatorTxRx(u8 *t_buf, u8 t_len, u8 *r_buf, u8 *r_len){

    Delay_Ms(15);
    nfc_buf[0] = PN532_COMMAND_INDATAEXCHANGE;
    nfc_buf[1] = 0x01; // logical number of the relevant target

    memcpy(nfc_buf+2, t_buf, t_len);

    if(!write_cmd_check_ack(nfc_buf, t_len+2)){
        return 0;
    }

    read_dt(nfc_buf, 60);
    if(nfc_buf[5] != 0xD5){
        return 0;
    }

    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_INDATAEXCHANGE+1)){

        return 0;
    }

    if(nfc_buf[NFC_FRAME_ID_INDEX+1]){

        return 0;
    }

    /** return read data */
    *r_len = nfc_buf[3]-3;
    memcpy(r_buf, nfc_buf+8, *r_len);
    return 1;
}

u8 P2PTargetInit(void){

    /** avoid resend command */
    static u8 send_flag=1;
    nfc_buf[0] = PN532_COMMAND_TGINITASTARGET;
    /** 14443-4A Card only */
    nfc_buf[1] = 0x00;

    /** SENS_RES */
    nfc_buf[2] = 0x04;
    nfc_buf[3] = 0x00;

    /** NFCID1 */
    nfc_buf[4] = 0x12;
    nfc_buf[5] = 0x34;
    nfc_buf[6] = 0x56;

    /** SEL_RES */
    nfc_buf[7] = 0x40;      // DEP only mode

    /**Parameters to build POL_RES (18 bytes including system code) */
    nfc_buf[8] = 0x01;
    nfc_buf[9] = 0xFE;
    nfc_buf[10] = 0xA2;
    nfc_buf[11] = 0xA3;
    nfc_buf[12] = 0xA4;
    nfc_buf[13] = 0xA5;
    nfc_buf[14] = 0xA6;
    nfc_buf[15] = 0xA7;
    nfc_buf[16] = 0xC0;
    nfc_buf[17] = 0xC1;
    nfc_buf[18] = 0xC2;
    nfc_buf[19] = 0xC3;
    nfc_buf[20] = 0xC4;
    nfc_buf[21] = 0xC5;
    nfc_buf[22] = 0xC6;
    nfc_buf[23] = 0xC7;
    nfc_buf[24] = 0xFF;
    nfc_buf[25] = 0xFF;
    /** NFCID3t */
    nfc_buf[26] = 0xAA;
    nfc_buf[27] = 0x99;
    nfc_buf[28] = 0x88;
    nfc_buf[29] = 0x77;
    nfc_buf[30] = 0x66;
    nfc_buf[31] = 0x55;
    nfc_buf[32] = 0x44;
    nfc_buf[33] = 0x33;
    nfc_buf[34] = 0x22;
    nfc_buf[35] = 0x11;
    /** Length of general bytes  */
    nfc_buf[36] = 0x00;
    /** Length of historical bytes  */
    nfc_buf[37] = 0x00;

    if(send_flag){
        send_flag = 0;
        if(!write_cmd_check_ack(nfc_buf, 38)){
            send_flag = 1;
            return 0;
        }
    }

    read_dt(nfc_buf, 24);

    if(nfc_buf[5] != 0xD5){
        return 0;
    }

    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_TGINITASTARGET+1)){
        return 0;
    }

    send_flag = 1;
    return 1;
}

u8 P2PTargetTxRx(u8 *t_buf, u8 t_len, u8 *r_buf, u8 *r_len){

    nfc_buf[0] = PN532_COMMAND_TGGETDATA;
    if(!write_cmd_check_ack(nfc_buf, 1)){
        return 0;
    }

    read_dt(nfc_buf, 60);
    if(nfc_buf[5] != 0xD5){
        return 0;
    }

    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_TGGETDATA+1)){
        return 0;
    }
    if(nfc_buf[NFC_FRAME_ID_INDEX+1]){
        return 0;
    }

    /** return read data */
    *r_len = nfc_buf[3]-3;
    memcpy(r_buf, nfc_buf+8, *r_len);

    nfc_buf[0] = PN532_COMMAND_TGSETDATA;
    memcpy(nfc_buf+1, t_buf, t_len);

    if(!write_cmd_check_ack(nfc_buf, 1+t_len)){
        return 0;
    }

    read_dt(nfc_buf, 26);

    if(nfc_buf[5] != 0xD5){
        return 0;
    }

    if(nfc_buf[NFC_FRAME_ID_INDEX] != (PN532_COMMAND_TGSETDATA+1)){
        return 0;
    }
    if(nfc_buf[NFC_FRAME_ID_INDEX+1]){
        return 0;
    }

    return 1;
}
