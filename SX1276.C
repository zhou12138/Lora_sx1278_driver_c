/*
�ļ�ʹ��˵����
�ⲿ�ļ�����Lora_Init()����lora�Ĳ����ͽӿڳ�ʼ��
ʹ��void Lora_Send(unsigned char *p_send_buf,unsigned char len);�������ݷ���
ʹ��void Lora_RecDataGet(unsigned char *p_recdata,unsigned char len);��������
ʹ��unsigned char Lora_GetNumofRecData(void);��ȡ���ܵ����ݵ��ֽڳ���
�ڶ�ӦIO���жϴ��������е���void SX1278_Interupt(void);����lora���ݴ���
		TxDone RxDone CADDone ```
		Lora module ��IO0 IO1 IO2 IO3 IO5��Ӧ���ⲿ����EXTI�ж� ����SX1278_Interupt()
*/
#include "Sx1276.h"
#include "stm32l1xx.h"
#include "string.h"

/* �궨��*/
#define LORA_RECNUM_MAX 512
/* GPIO��غ궨�� */
#define SPI 			            SPI1
#define SPI_CLK 			        RCC_APB2Periph_SPI1

#define SPI_SCK_PIN 		        GPIO_Pin_5
#define SPI_SCK_GPIO_PORT 		    GPIOA
#define SPI_SCK_GPIO_CLK 		    RCC_AHBPeriph_GPIOA

#define SPI_MISO_PIN 		        GPIO_Pin_6
#define SPI_MISO_GPIO_PORT 		    GPIOA
#define SPI_MISO_GPIO_CLK 		    RCC_AHBPeriph_GPIOA

#define SPI_MOSI_PIN 		        GPIO_Pin_7
#define SPI_MOSI_GPIO_PORT 		    GPIOA
#define SPI_MOSI_GPIO_CLK 		    RCC_AHBPeriph_GPIOA

#define SPI_CS_PIN 			        GPIO_Pin_4
#define SPI_CS_GPIO_PORT 		    GPIOA
#define SPI_CS_GPIO_CLK 		    RCC_AHBPeriph_GPIOA

#define LORA_INT1_PIN			    GPIO_Pin_2
#define LORA_INT1_GPIO_PORT 		GPIOC
#define LORA_INT1_GPIO_CLK 		    RCC_AHBPeriph_GPIOC

#define LORA_INT2_PIN			    GPIO_Pin_3
#define LORA_INT2_GPIO_PORT 		GPIOC
#define LORA_INT2_GPIO_CLK 		    RCC_AHBPeriph_GPIOC

#define LORA_INT3_PIN			    GPIO_Pin_4
#define LORA_INT3_GPIO_PORT 		GPIOC
#define LORA_INT3_GPIO_CLK 		    RCC_AHBPeriph_GPIOC

#define LORA_INT4_PIN			    GPIO_Pin_5
#define LORA_INT4_GPIO_PORT 		GPIOC
#define LORA_INT4_GPIO_CLK 		    RCC_AHBPeriph_GPIOC

#define LORA_INT5_PIN			    GPIO_Pin_6
#define LORA_INT5_GPIO_PORT 		GPIOC
#define LORA_INT5_GPIO_CLK 		    RCC_AHBPeriph_GPIOC

#define LORA_RST_PIN			    GPIO_Pin_7
#define LORA_RST_GPIO_PORT 		    GPIOC
#define LORA_RST_GPIO_CLK 		    RCC_AHBPeriph_GPIOC

#define LORA_RXMODE_PIN			    GPIO_Pin_8
#define LORA_RXMODE_GPIO_PORT 		GPIOC
#define LORA_RXMODE_GPIO_CLK 		RCC_AHBPeriph_GPIOC

#define LORA_TXMODE_PIN			    GPIO_Pin_9
#define LORA_TXMODE_GPIO_PORT 		GPIOC
#define LORA_TXMODE_GPIO_CLK 		RCC_AHBPeriph_GPIOC
/*
#define LORA_IRQ1_HANDLE            EXTI2_IRQHandler
#define LORA_IRQ2_HANDLE            EXTI3_IRQHandler
#define LORA_IRQ3_HANDLE            EXTI4_IRQHandler
#define LORA_IRQ4_HANDLE            EXTI9_5_IRQHandler
*/
#define  SX1278_SDO                         GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6)

#define  RF_REST_L			    								GPIO_ResetBits(GPIOC, GPIO_Pin_7)	     
#define  RF_REST_H			    								GPIO_SetBits(GPIOC, GPIO_Pin_7)	  

#define  RF_CE_L                            GPIO_ResetBits(GPIOA, GPIO_Pin_4)      
#define  RF_CE_H                            GPIO_SetBits(GPIOA, GPIO_Pin_4)         

#define  RF_CKL_L                           GPIO_ResetBits(GPIOA, GPIO_Pin_5)     
#define  RF_CKL_H                           GPIO_SetBits(GPIOA, GPIO_Pin_5)      

#define  RF_SDI_L                           GPIO_ResetBits(GPIOA, GPIO_Pin_7)      
#define  RF_SDI_H                           GPIO_SetBits(GPIOA, GPIO_Pin_7)         

#define  PA_TXD_OUT()                       GPIO_SetBits(GPIOC, GPIO_Pin_9);\
                                            GPIO_ResetBits(GPIOC, GPIO_Pin_8)
#define  PA_RXD_OUT()                       GPIO_SetBits(GPIOC, GPIO_Pin_8);\
                                            GPIO_ResetBits(GPIOC, GPIO_Pin_9)  

/*lora ��ʼ����������*/
#define   SPREADINGFACTOR  11    //7-12
#define   CODINGRATE    2        //1-4
#define   BW_FREQUENCY  7        //6-9
#define   POWERVALUE    7
const unsigned char  power_data[8]={0X80,0X80,0X80,0X83,0X86,0x89,0x8c,0x8f};
unsigned char   Frequency[3]={0x6c,0x80,0x00};

/* ȫ�ֱ��� ���� */
unsigned char   SX1278_RLEN;
unsigned char   recv[LORA_RECNUM_MAX];

lpCtrlTypefunc_t lpTypefunc = {0,0,0,0};

/*************************************************************
  Function   ��Lora_GPIO_Config  
  Description��lora module��Ӧ��MCU�ӿڶ���
	Input      : none
  return     : none 
*************************************************************/	
void Lora_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	
	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOC , ENABLE );
	
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin   = SPI_SCK_PIN | SPI_MOSI_PIN | SPI_CS_PIN ;
  GPIO_Init( SPI_CS_GPIO_PORT, &GPIO_InitStructure );
  GPIO_SetBits(SPI_CS_GPIO_PORT, SPI_CS_PIN);
  
  GPIO_InitStructure.GPIO_Pin   = LORA_RXMODE_PIN | LORA_TXMODE_PIN | LORA_RST_PIN;
  GPIO_Init( LORA_RXMODE_GPIO_PORT, &GPIO_InitStructure );
   
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin   = LORA_RST_PIN;
  GPIO_Init( LORA_RST_GPIO_PORT, &GPIO_InitStructure );
   
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin   = SPI_MISO_PIN;
  GPIO_Init( SPI_MISO_GPIO_PORT, &GPIO_InitStructure ); 
   
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin   = LORA_INT1_PIN;
  GPIO_Init( LORA_INT1_GPIO_PORT, &GPIO_InitStructure );
  
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin   = LORA_INT2_PIN;
  GPIO_Init( LORA_INT2_GPIO_PORT, &GPIO_InitStructure );
    
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin   = LORA_INT3_PIN;
  GPIO_Init( LORA_INT3_GPIO_PORT, &GPIO_InitStructure );
    
 /*GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin   = LORA_INT4_PIN;
  GPIO_Init( LORA_INT4_GPIO_PORT, &GPIO_InitStructure );
    
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin   = LORA_INT5_PIN;
  GPIO_Init( LORA_INT5_GPIO_PORT, &GPIO_InitStructure );
*/
		
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);  		

  NVIC_InitStructure.NVIC_IRQChannel 		             = EXTI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd 	             = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel 		             = EXTI3_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel 		             = EXTI4_IRQn;
  NVIC_Init(&NVIC_InitStructure);
    
/*
	NVIC_InitStructure.NVIC_IRQChannel 		             = EXTI9_5_IRQn;
  NVIC_Init(&NVIC_InitStructure);
*/
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource2);  
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource3);
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource4);
/* SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource5);
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource6);	*/

  EXTI_InitStructure.EXTI_Line    = EXTI_Line2;
  EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
	EXTI_InitStructure.EXTI_Line    = EXTI_Line3;
  EXTI_Init(&EXTI_InitStructure);
    
  EXTI_InitStructure.EXTI_Line    = EXTI_Line4;
  EXTI_Init(&EXTI_InitStructure);
    
/*EXTI_InitStructure.EXTI_Line    = EXTI_Line5;
  EXTI_Init(&EXTI_InitStructure);
     
  EXTI_InitStructure.EXTI_Line    = EXTI_Line6;
  EXTI_Init(&EXTI_InitStructure);*/
    
  EXTI_ClearITPendingBit(EXTI_Line2);
  EXTI_ClearITPendingBit(EXTI_Line3);
  EXTI_ClearITPendingBit(EXTI_Line4);
/*EXTI_ClearITPendingBit(EXTI_Line5);
	EXTI_ClearITPendingBit(EXTI_Line6);*/
}

/*************************************************************
  Function   ��SX1276Reset  
  Description��lora module ��λ����
							        __      ____
               RESET :  |____|  
	Input      : none
  return     : none 
*************************************************************/
void SX1276Reset(void)
{
   RF_REST_L;	
   Delay1s(200);
   RF_REST_H;
   Delay1s(500);
}
/*************************************************************
  Function   ��RF_SPI_WRITE_BYTE  
  Description��SPI д����
	Input      : data SPIд������� 
  return     : none 
*************************************************************/
void RF_SPI_WRITE_BYTE(unsigned char data)
{
   unsigned char i;
   for (i=0;i<8;i++){   
     if (data & 0x80)						/* check if MSB is high */
       RF_SDI_H;
     else RF_SDI_L;							/* if not, set to low */
		 
     RF_CKL_H;						  		/* toggle clock high */
     data = (data << 1);				/* shift 1 place for next bit */
     RF_CKL_L;									/* toggle clock low */
   }
}
/*************************************************************
  Function   ��RF_SPI_READ_BYTE  
  Description��SPI ������
	Input      : none
  return     : ��ȡ������ 
*************************************************************/
unsigned char RF_SPI_READ_BYTE()
{	 
   unsigned char j;
   unsigned char i;
   j=0;
   for (i = 0; i < 8; i++){	 
     RF_CKL_H; 
     j = (j << 1);						 // shift 1 place to the left or shift in 0 //
     if( SX1278_SDO )							 // check to see if bit is high //
       j = j | 0x01; 					   // if high, make bit high //
												  // toggle clock high // 
     RF_CKL_L; 							 // toggle clock low //  
   }
  
   return j;								// toggle clock low //
}
/*************************************************************
  Function   ��cmdSwitchEn  
  Description��SPI chip select
	Input      : cmdEntype 
								enClose  --- disable chip
								enOpen   --- enable chip
  return     : 
*************************************************************/
void cmdSwitchEn(cmdEntype_t cmd)
{
   switch(cmd)
   {
     case enOpen:{
       RF_CE_L;
     }break;
     case enClose:{
       RF_CE_H;
     }break;
     default:break;
   }
}
/*************************************************************
  Function   ��cmdSwitchEn  
  Description��SPI chip select
	Input      : cmdpatype 
								rxOpen  --- rx enable
								txOpen   ---tx enable
  return     : 
*************************************************************/
void cmdSwitchPA(cmdpaType_t cmd)
{
   switch(cmd)
   {
     case rxOpen:{
       PA_RXD_OUT();
     }break;
     case txOpen:{
       PA_TXD_OUT();
     }break;
     
     default:break;
   }
}


lpCtrlTypefunc_t  ctrlTypefunc = {
   RF_SPI_WRITE_BYTE,
   RF_SPI_READ_BYTE,
   cmdSwitchEn,
   cmdSwitchPA
};
void Delay1s(unsigned int ii)
{
   unsigned char j;
   while(ii--){
     for(j=0;j<100;j++);
   }
}
/*************************************************************
  Function   ��register_rf_func  
  Description���ṹ�������ֵ
	Input      : func lpCtrlTypefunc_t���ͱ��� 
							 ����ֵ��ȫ�ֱ���lpTypefunc
  return     : none  
*************************************************************/	
void register_rf_func(lpCtrlTypefunc_t *func)
{
   if(func->lpByteWritefunc != 0){
      lpTypefunc.lpByteWritefunc = func->lpByteWritefunc;
   }
   if(func->lpByteReadfunc != 0){
      lpTypefunc.lpByteReadfunc = func->lpByteReadfunc;
   }
   if(func->lpSwitchEnStatus != 0){
      lpTypefunc.lpSwitchEnStatus = func->lpSwitchEnStatus;
   }
   if(func->paSwitchCmdfunc != 0){
      lpTypefunc.paSwitchCmdfunc = func->paSwitchCmdfunc;
   }
}
/*************************************************************
  Function   ��SX1276WriteBuffer  
  Description��SPI ���ַΪaddr�ļĴ���д������buffer
	Input      : addr sx1278�ļĴ�����ַ	
							 buffer д�������
  return     : none  
*************************************************************/	
void SX1276WriteBuffer( unsigned char addr, unsigned char buffer)
{ 
   lpTypefunc.lpSwitchEnStatus(enOpen); //NSS = 0;
   lpTypefunc.lpByteWritefunc( addr | 0x80 );
   lpTypefunc.lpByteWritefunc( buffer);
   lpTypefunc.lpSwitchEnStatus(enClose); //NSS = 1;
}
/*************************************************************
  Function   ��SX1276ReadBuffer  
  Description��SPI ��ȡ��ַΪaddr�ļĴ�������
	Input      : addr sx1278�ļĴ�����ַ							
  return     : �Ĵ�����ŵ�����  
*************************************************************/		
unsigned char SX1276ReadBuffer(unsigned char addr)
{
  unsigned char Value;
  lpTypefunc.lpSwitchEnStatus(enOpen); //NSS = 0;
  lpTypefunc.lpByteWritefunc( addr & 0x7f  );
  Value = lpTypefunc.lpByteReadfunc();
  lpTypefunc.lpSwitchEnStatus(enClose);//NSS = 1;

  return Value; 
}
/*************************************************************
  Function   ��SX1276LoRaFsk  
  Description��lora������������ ͨ�Ź���ģʽ TX RX CAD SLEEP IDLE
	Input      : opmode RFMode_SEt����							
  return     : none   
*************************************************************/		
void SX1276LoRaSetOpMode( RFMode_SET opMode )
{
   unsigned char opModePrev;
   opModePrev=SX1276ReadBuffer(REG_LR_OPMODE);
   opModePrev &=0xf8;
   opModePrev |= (unsigned char)opMode;
   SX1276WriteBuffer( REG_LR_OPMODE, opModePrev);		
}
/*************************************************************
  Function   ��SX1276LoRaFsk  
  Description��lora������������ ͨ��ģʽ FSK ����lora
	Input      : opmode
							FSK_mode | LORA_mode
  return     : none   
*************************************************************/	
void SX1276LoRaFsk( Debugging_fsk_ook opMode )
{
   unsigned char opModePrev;
   opModePrev=SX1276ReadBuffer(REG_LR_OPMODE);
   opModePrev &=0x7F;
   opModePrev |= (unsigned char)opMode;
   SX1276WriteBuffer( REG_LR_OPMODE, opModePrev);		
}
/*************************************************************
  Function   ��SX1276LoRaSetRFFrequency  
  Description��lora������������ ͨ��Ƶ��
							frequency = F(xosc)*Frf/2^19
	Input      : Frequency[i]
							Resolution is 61.035 Hz if F(XOSC) = 32 MHz. Default value is
							0x6c8000 = 434 MHz. Register values must be modified only
							when device is in SLEEP or STAND-BY mode.
  return     : none   
*************************************************************/		
void SX1276LoRaSetRFFrequency(void)
{
   SX1276WriteBuffer( REG_LR_FRFMSB, Frequency[0]);
   SX1276WriteBuffer( REG_LR_FRFMID, Frequency[1]);
   SX1276WriteBuffer( REG_LR_FRFLSB, Frequency[2]);
}
/*************************************************************
  Function   ��SX1276LoRaSetRFPower  
  Description��lora������������ ���书������
							PA_BOOST pin.
							Maximum power of +20 dBm
							maxpower  pmax=10.8+0.6*maxpower[dbm]
							Pout=17-(15-OutputPower)
	Input      : power :
							{0X80,0X80,0X80,0X83,0X86,0x89,0x8c,0x8f}&0x0F;
  return     : none   
*************************************************************/		
void SX1276LoRaSetRFPower(unsigned char power )
{
   SX1276WriteBuffer( REG_LR_PADAC, 0x87);
   SX1276WriteBuffer( REG_LR_PACONFIG,  power_data[power] );
}
/*************************************************************
  Function   ��SX1276LoRaSetSpreadingFactor  
  Description��lora������������  ��Ƶ��������
	Input      : value :
								SF rate (expressed as a base-2 logarithm)
								6 --- 64 chips / symbol
								7 --- 128 chips / symbol
								8 --- 256 chips / symbol
								9 --- 512 chips / symbol
								10 --- 1024 chips / symbol
								11 --- 2048 chips / symbol
								12 --- 4096 chips / symbol
								other values reserved
  return     : none   
*************************************************************/
void SX1276LoRaSetSpreadingFactor(unsigned char factor )
{
   unsigned char RECVER_DAT;
   SX1276LoRaSetNbTrigPeaks( 3 );
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG2);	  
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG2_SF_MASK ) | ( factor << 4 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG2, RECVER_DAT );	 
}
	
/*************************************************************
  Function   ��SX1276LoRaSetNbTrigPeaks  
  Description��lora������������ 
								2-0 bits of register 0x31
	Input      : value :

  return     : none   
*************************************************************/
void SX1276LoRaSetNbTrigPeaks(unsigned char value )
{
   unsigned char RECVER_DAT;
   RECVER_DAT = SX1276ReadBuffer( 0x31);
   RECVER_DAT = ( RECVER_DAT & 0xF8 ) | value;
   SX1276WriteBuffer( 0x31, RECVER_DAT );
}
/*************************************************************
  Function   ��SX1276LoRaSetErrorCoding  
  Description��lora������������ error coding rate
							In implicit header mode should be set on receiver to determine
							expected coding rate.
	Input      : value :
									001  ----  4/5
									010  ----  4/6
									011  ----  4/7
									100  ----  4/8	
  return     : none   
*************************************************************/
void SX1276LoRaSetErrorCoding(unsigned char value )
{	
   unsigned char RECVER_DAT;
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG1);
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( value << 1 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG1, RECVER_DAT);
}
/*************************************************************
  Function   ��SX1276LoRaSetPacketCrcOn  
  Description��lora������������ packet crc on
	Input      : enabe :
									true  --- enable
									false --- disable
  return     : none   
*************************************************************/
void SX1276LoRaSetPacketCrcOn(BOOL enable )
{	
   unsigned char RECVER_DAT;
   RECVER_DAT= SX1276ReadBuffer( REG_LR_MODEMCONFIG2);
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( enable << 2 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG2, RECVER_DAT );
}

/*************************************************************
  Function   ��SX1276LoRaSetSignalBandwidth  
  Description��lora������������ signal bandwidth
  Input      : bw --- 
								 0000  ----.  7.8khz
								 0001  ----.  10.4khz
								 0010  ----.  15.6khz
								 0011  ----.  20.8khz
								 0100  ----.  31.25khz
								 0101  ----.  41.7khz
								 0110  ----.  62.5khz
								 0111  ----.  125Khz
								 1000  ----.  250khz
								 1001  ----.  500khz

  return     : none   
*************************************************************/
void SX1276LoRaSetSignalBandwidth(unsigned char bw )
{
   unsigned char RECVER_DAT;
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG1);
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG1_BW_MASK ) | ( bw << 4 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG1, RECVER_DAT );
}
	
/*************************************************************
  Function   ��SX1276LoRaSetImplicitHeaderOn  
  Description��lora������������ ImplicitHeaderOn
  Input      : enabe --- 
									true implicit header mode
									false  explicit header mode
  return     : none   
*************************************************************/
void SX1276LoRaSetImplicitHeaderOn(BOOL enable )
{
   unsigned char RECVER_DAT;
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG1 );
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( enable );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG1, RECVER_DAT );
}
	
/*************************************************************
  Function   ��SX1276LoRaSetSymbTimeout  
  Description��lora������������ ����Rx operation time-out value 
							Timeout =  symboltimeout.Ts
  Input      : value  9bit valid 
  return     : none   
*************************************************************/
void SX1276LoRaSetSymbTimeout(unsigned int value )
{
   unsigned char RECVER_DAT[2];
   RECVER_DAT[0]=SX1276ReadBuffer( REG_LR_MODEMCONFIG2 );
   RECVER_DAT[1]=SX1276ReadBuffer( REG_LR_SYMBTIMEOUTLSB );
   RECVER_DAT[0] = ( RECVER_DAT[0] & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
   RECVER_DAT[1] = value & 0xFF;
   SX1276WriteBuffer( REG_LR_MODEMCONFIG2, RECVER_DAT[0]);
   SX1276WriteBuffer( REG_LR_SYMBTIMEOUTLSB, RECVER_DAT[1]);
}
	
/*************************************************************
  Function   ��SX1276LoRaSetPayloadLength  
  Description��lora������������ ����payload length
  Input      : value  ��0�� ������ 
  return     : none   
*************************************************************/
void SX1276LoRaSetPayloadLength(unsigned char value )
{
   SX1276WriteBuffer( REG_LR_PAYLOADLENGTH, value );
} 
	
/*************************************************************
  Function   ��SX1276LoRaSetPreamLength  
  Description��lora������������ ����preamble length
  Input      : value ���ֽ�����MSB ���ֽ�����LSB MSB=preamble length +4.25symbol
  return     : none   
*************************************************************/
void SX1276LoRaSetPreamLength(unsigned int value )
{
   unsigned char a[2];
   a[0]=(value&0xff00)>>8;
   a[1]=value&0xff;
   SX1276WriteBuffer( REG_LR_PREAMBLEMSB, a[0] );
   SX1276WriteBuffer( REG_LR_PREAMBLELSB, a[1] );
}
	
/*************************************************************
  Function   ��SX1276LoRaSetMobileNode  
  Description��lora������������ �����Ż���
  Input      : Bool true-�Ż� false --���Ż�
  return     : none   
*************************************************************/
void SX1276LoRaSetMobileNode(BOOL enable )
{	 
   unsigned char RECVER_DAT;
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG3 );
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG3_MOBILE_NODE_MASK ) | ( enable << 3 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG3, RECVER_DAT );
}

/*************************************************************
  Function   ��SX1276LORA_INT  
  Description��lora��ʼ��,ͨ�Ų���
  Input      : none 
  return     : none   
*************************************************************/
void SX1276LORA_INT(void)
{
   SX1276LoRaSetOpMode(Sleep_mode);  									//����˯��ģʽ
   SX1276LoRaFsk(LORA_mode);	     										// ������Ƶģʽ
   SX1276LoRaSetOpMode(Stdby_mode);  									// ����Ϊ��ͨģʽ
   SX1276WriteBuffer( REG_LR_DIOMAPPING1,GPIO_VARE_1);// IO ��־���� IO 0 1 2 3
   SX1276WriteBuffer( REG_LR_DIOMAPPING2,GPIO_VARE_2); // IO 5
   SX1276LoRaSetRFFrequency();								        //����Ƶ�� 434MHZ
   SX1276LoRaSetRFPower(POWERVALUE);					        // ���ù���
   SX1276LoRaSetSpreadingFactor(SPREADINGFACTOR);	    // ��Ƶ��������
   SX1276LoRaSetErrorCoding(CODINGRATE);		          //��Ч���ݱ�
   SX1276LoRaSetPacketCrcOn(true);			              //CRC У���
   SX1276LoRaSetSignalBandwidth( BW_FREQUENCY );	    //������Ƶ����
   SX1276LoRaSetImplicitHeaderOn(false);		          //ͬ��ͷ������ģʽ
   SX1276LoRaSetPayloadLength( 0xff);									//���payload length 255
   SX1276LoRaSetSymbTimeout( 0x3FF );									//��ʱ����
   SX1276LoRaSetMobileNode(true); 			             // �����ݵ��Ż�
   RF_RECEIVE(); 																		//����RX ģʽ
}
/*************************************************************
  Function   ��FUN_RF_SENDPACKET  
  Description��lora ����һ�����ȵ�����
  Input      : RF_TRAN_P ---������������ LEN--���������ݳ���
  return     : none   
*************************************************************/	
void FUN_RF_SENDPACKET(unsigned char *RF_TRAN_P,unsigned char LEN)
{
   unsigned char ASM_i;
   lpTypefunc.paSwitchCmdfunc(txOpen);
   SX1276LoRaSetOpMode( Stdby_mode );
   SX1276WriteBuffer( REG_LR_HOPPERIOD, 0 );								//����Ƶ������
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,IRQN_TXD_Value);		//�򿪷����ж�
   SX1276WriteBuffer( REG_LR_PAYLOADLENGTH, LEN);	 					//������ݰ�
   SX1276WriteBuffer( REG_LR_FIFOTXBASEADDR, 0);
   SX1276WriteBuffer( REG_LR_FIFOADDRPTR, 0 );
   lpTypefunc.lpSwitchEnStatus(enOpen);
   lpTypefunc.lpByteWritefunc( 0x80 );
   for( ASM_i = 0; ASM_i < LEN; ASM_i++ ){
     lpTypefunc.lpByteWritefunc( *RF_TRAN_P );RF_TRAN_P++;
   }
   lpTypefunc.lpSwitchEnStatus(enClose);
   SX1276WriteBuffer(REG_LR_DIOMAPPING1,0x40);
   SX1276WriteBuffer(REG_LR_DIOMAPPING2,0x00);
   SX1276LoRaSetOpMode( Transmitter_mode );
}

/*************************************************************
  Function   ��RF_RECEIVE  
  Description��lora ����ģʽ ���մ� �жϷ�ʽ
  Input      : none 
  return     : none   
*************************************************************/
void RF_RECEIVE (void)
{
   SX1276LoRaSetOpMode(Stdby_mode );
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,IRQN_RXD_Value);  //��Rx Done�ж�
   SX1276WriteBuffer(REG_LR_HOPPERIOD,	PACKET_MIAX_Value );
   SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X00 );
   SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0X00 );	
   SX1276LoRaSetOpMode( Receiver_mode );
   lpTypefunc.paSwitchCmdfunc(rxOpen);
}
	
/*************************************************************
  Function   ��RF_CAD_RECEIVE  
  Description��lora CAD ��� ���մ� 
  Input      : none 
  return     : none   
*************************************************************/
void RF_CAD_RECEIVE (void)
{
   SX1276LoRaSetOpMode( Stdby_mode );
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,  IRQN_CAD_Value);	//��CAD Done �ж�
   SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X80 );
   SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0X00 );	
   SX1276LoRaSetOpMode( CAD_mode );
   lpTypefunc.paSwitchCmdfunc(rxOpen);
}
	
/*************************************************************
  Function   ��RF_SLEEP  
  Description��lora sleepģʽ���� 
  Input      : none 
  return     : none   
*************************************************************/
void RF_SLEEP(void)
{
   SX1276LoRaSetOpMode( Stdby_mode );
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,  IRQN_SLEEP_Value);  //��Sleep�ж�
   SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X00 );
   SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0X00 );	
   SX1276LoRaSetOpMode( Sleep_mode );
}

//**************�������ж����洦���Ĵ���**********************
/*************************************************************
  Function   ��SX1278_Interupt  
  Description��lora��Ӧ�жϱ�־������TXDONE RXDONE CADDONE ```
  Input      : none 
  return     : none   
*************************************************************/
void SX1278_Interupt(void)
{
	unsigned char RF_REC_RLEN_i =0;
	unsigned char   CRC_Value;
	unsigned char   RF_EX0_STATUS;
	
  RF_EX0_STATUS=SX1276ReadBuffer( REG_LR_IRQFLAGS ); 
	
  if((RF_EX0_STATUS&0x40)==0x40){														//���� RX Done
    CRC_Value=SX1276ReadBuffer( REG_LR_MODEMCONFIG2 );
    if(CRC_Value&0x04==0x04){
      SX1276WriteBuffer (REG_LR_FIFOADDRPTR,0x00);
      SX1278_RLEN = SX1276ReadBuffer(REG_LR_NBRXBYTES);
      lpTypefunc.lpSwitchEnStatus(enOpen);
      lpTypefunc.lpByteWritefunc( 0x00 );
      for(RF_REC_RLEN_i=0;RF_REC_RLEN_i<SX1278_RLEN;RF_REC_RLEN_i++){
        recv[RF_REC_RLEN_i]=lpTypefunc.lpByteReadfunc();	
							
      }
      lpTypefunc.lpSwitchEnStatus(enClose);
    }       
    SX1276LoRaSetOpMode( Stdby_mode );
    SX1276WriteBuffer(REG_LR_IRQFLAGSMASK, IRQN_RXD_Value);  //��RXD�ж�
    SX1276WriteBuffer(REG_LR_HOPPERIOD,    PACKET_MIAX_Value);
    SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X00 );
    SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0x00 );	
    SX1276LoRaSetOpMode( Receiver_mode );
    lpTypefunc.paSwitchCmdfunc(rxOpen);
  }else if((RF_EX0_STATUS&0x08)==0x08){											// TX Done
    SX1276LoRaSetOpMode( Stdby_mode );
    SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,IRQN_RXD_Value);  //��RXD�ж�
    SX1276WriteBuffer(REG_LR_HOPPERIOD,   PACKET_MIAX_Value );
    SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X00 );
    SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0x00 );	
    SX1276LoRaSetOpMode( Receiver_mode );
    lpTypefunc.paSwitchCmdfunc(rxOpen);
  }else if((RF_EX0_STATUS&0x04)==0x04){  										// CAD Done
    if((RF_EX0_STATUS&0x01)==0x01){     //��ʾCAD ��⵽��Ƶ�ź� ģ������˽���״̬����������
      SX1276LoRaSetOpMode( Stdby_mode );
      SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,IRQN_RXD_Value);  //��RXD�ж�
      SX1276WriteBuffer(REG_LR_HOPPERIOD,   PACKET_MIAX_Value );
      SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X02 );
      SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0x00 );	
      SX1276LoRaSetOpMode( Receiver_mode );
      lpTypefunc.paSwitchCmdfunc(rxOpen);
    }else{                           // û��⵽
      SX1276LoRaSetOpMode( Stdby_mode );
      SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,  IRQN_SLEEP_Value);  //��sleep�ж�
      SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X00 );
      SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0X00 );	
      SX1276LoRaSetOpMode( Sleep_mode );
    }
  }else{
    SX1276LoRaSetOpMode( Stdby_mode );
    SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,IRQN_RXD_Value);  //��RXD�ж�
    SX1276WriteBuffer(REG_LR_HOPPERIOD,   PACKET_MIAX_Value );
    SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X02 );
    SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0x00 );	
    SX1276LoRaSetOpMode( Receiver_mode );
    lpTypefunc.paSwitchCmdfunc(rxOpen);
  }
  SX1276WriteBuffer( REG_LR_IRQFLAGS, 0xff  );
}
/*************************************************************
  Function   ��Lora_Init  
  Description��lora��ʼ����GPIO ��ͨ�Ų�������λ
  Input      : none 
  return     : none   
*************************************************************/
void Lora_Init(void)
{
	 Lora_GPIO_Config();
	 register_rf_func(&ctrlTypefunc);
   SX1276Reset();
   SX1276LORA_INT();
}
/*************************************************************
  Function   ��Lora_Send  
  Description������һ�����ȵ�����
  Input      : p_send_buf -����������buffer  len --���������ݳ���
  return     : none    
*************************************************************/
void Lora_Send(unsigned char *p_send_buf,unsigned char len)
{
	FUN_RF_SENDPACKET(p_send_buf,len);
}
void Lora_RecDataGet(unsigned char *p_recdata,unsigned char len)
{
	memcpy(p_recdata,recv,len);
	memset(recv,0,len);
}

/*************************************************************
  Function   ��Lora_GetNumofRecData  
  Description����ȡ���յ����ݳ���
  Input      : ��  
  return     : len-�������ݳ���    
*************************************************************/
unsigned char Lora_GetNumofRecData(void)
{
	unsigned char len;
	
	if(SX1278_RLEN) 
	{
		len = SX1278_RLEN;
		SX1278_RLEN = 0;
	}
	else 
	{
		len = 0;
	}
	return len;	
}
