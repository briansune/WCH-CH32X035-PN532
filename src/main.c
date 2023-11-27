#include "debug.h"
#include "nfc.h"

//#define CARD_READ
#define INIT_NFC
//#define TARG_NFC

#if(defined INIT_NFC || defined TARG_NFC)
u8 tx_buf[50]="NFC INITIATOR0";
u8 tx_len;
u8 rx_buf[50];
u8 rx_len;
#endif

int main(void){

#ifdef CARD_READ
    u8 buf[32], sta;
#endif

    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init( 115200 );
    printf( "SystemClk:%d\r\n", SystemCoreClock );
    printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );

    nfc_init();
    printf("PN532 Arduino Conversion\r\n");

    u32 versiondata = get_version();

    if (! versiondata) {
        printf("Didn't find PN53x board");
        while(1);
    }

    printf("Found chip PN5");
    printf("%x", (versiondata>>24) & 0xFF);
    printf("Firmware Ver. ");
    printf("%d", (versiondata>>16) & 0xFF);
    printf(".%d\r\n", (versiondata>>8) & 0xFF);

    u8 flag;
    flag = CiuRfConfiguration(1, 0, 12);
    printf("%d\r\n", flag);

    SAMConfiguration(PN532_SAM_NORMAL_MODE, 1, 0);

	while(1){

#ifdef CARD_READ
        /** Polling the mifar card, buf[0] is the length of the UID */
        sta = InListPassiveTarget(buf, PN532_BRTY_ISO14443A, 1, NULL);

        /** check state and UID length */
        if(sta && buf[0] == 4){
        /** the card may be Mifare Classic card, try to read the block */
        printf("UUID length:");
        printf("%d", buf[0]);
        printf("\r\n");
        printf("UUID:");
        puthex(buf+1, buf[0]);
        printf("\r\n");
        /** factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF */
        u8 key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        u8 blocknum = 4;
        /** Authentication blok 4 */
        sta = MifareAuthentication(0, blocknum, buf+1, buf[0], key);
        if(sta){
        /** save read block data */
        u8 block[16];
        printf("Authentication success.\r\n");

        /** read block 4 */
        sta = MifareReadBlock(blocknum, block);
        if(sta){
        printf("Read block successfully:");

        puthex(block, 16);
        printf("\r\n");
        }

        /** read block 5 */
        sta = MifareReadBlock(blocknum+1, block);
        if(sta){
        printf("Read block successfully:");

        puthex(block, 16);
        printf("\r\n");
        }

        /** read block 6 */
        sta = MifareReadBlock(blocknum+2, block);
        if(sta){
        printf("Read block successfully:");

        puthex(block, 16);
        printf("\r\n");
        }

        /** read block 7 */
        sta = MifareReadBlock(blocknum+3, block);
        if(sta){
        printf("Read block successfully:");

        puthex(block, 16);
        printf("\r\n");
        }
        }
        }
#endif

#ifdef INIT_NFC
        if(P2PInitiatorInit()){
            printf("Target is sensed.\r\n");

            tx_buf[13] = 0x30;
            u8 idx = 0;

            tx_len = strlen((const char*)tx_buf);
            while(P2PInitiatorTxRx(tx_buf, tx_len, rx_buf, &rx_len)){
                printf("Data Received: ");
                printf(rx_buf, rx_len);
                printf("\r\n");
                idx++;
                tx_buf[13] = 0x30 + (idx % 10);
            }
            printf("\r\n");
            Delay_Ms(3);
        }
#endif

#ifdef TARG_NFC
        if(P2PTargetInit()){
            printf("Initiator is sensed.\r\n");
            tx_len = strlen((const char *)tx_buf);
            if(P2PTargetTxRx(tx_buf, tx_len, rx_buf, &rx_len)){
                printf("Data Received: ");
                printf(rx_buf, rx_len);
                printf("\r\n");
            }
            printf("\r\n");
        }
#endif

	}
}
