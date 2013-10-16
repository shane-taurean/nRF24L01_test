/*******************************************************************************
*	Test code for a nrf24L01 radio											   *
*																			   *
*	Intended use: to control a catamaran type rc boat						   *
*																			   *
*	The following links proved helpful in using the nrf24L01				   *
*	NRF24L01 With Arduinio�s SPI Library http://www.elecfreaks.com/480.html	   *
*	NRF24L01 Module Demo For Arduino http://www.elecfreaks.com/203.html		   *
*																			   *	
*																			   *
*	CS		- Arduino digital pin 8,  nRF24L01 pin 3 (CE)					   *
*	CSN 	- Arduino digital pin 9,  nRF24L01 pin 4 (CSN)					   *
*	MOSI 	- Arduino digital pin 11, nRF24L01 pin 6 (MOSI) 				   *
*	MISO 	- Arduino digital pin 12, nRF24L01 pin 7 (MISO)					   *
*	CLK 	- Arduino digital pin 13, nRF24L01 pin 5 (SCK) 					   *
*																			   *
/******************************************************************************/

#include <SPI.h>
#include "nRF24L01.h"

#define TX_ADR_WIDTH    5   
#define TX_PLOAD_WIDTH  32

#define CE       		8
#define CSN      		9
#define IRQ      		10

unsigned char TX_ADDRESS[TX_ADR_WIDTH]  = 
{
  0x34,0x43,0x10,0x10,0x01
}; // Define a static TX address

unsigned char rx_buf[TX_PLOAD_WIDTH] = {0}; // initialize value
unsigned char tx_buf[TX_PLOAD_WIDTH] = {0};

//**************************************************
void setup() 
{
  Serial.begin(9600);
  pinMode(CE,  OUTPUT);
  pinMode(CSN, OUTPUT);
  pinMode(IRQ, INPUT);
  SPI.begin();
  delay(50);
  init_io();                       	// Initialize IO port
  unsigned char sstatus=SPI_Read(STATUS);
  Serial.println("*** Get Status ***");
  Serial.print("status = ");    
  Serial.println(sstatus,HEX);		// read the modes status register, 
									//the default value should be 'E'
  Serial.println("******************");
  //TX_Mode();                  	// set TX mode
}

//**************************************************
void loop() 
{
  int k = 0;
  while ( 1 )
  {
    for(int i=0; i<32; i++)
        tx_buf[i] = k++;        
    unsigned char sstatus = SPI_Read(STATUS);	// read register STATUS's value
    if(sstatus&TX_DS)							// Data Sent TX FIFO interrupt, packet transmitted on TX
    {
      SPI_RW_Reg(FLUSH_TX,0);                                  
      SPI_Write_Buf(W_TX_PAYLOAD,tx_buf,TX_PLOAD_WIDTH);	// write playload to TX_FIFO
    }
	/*
	//???? --- review this section
    if(sstatus&MAX_RT)			// if receive data ready (MAX_RT) interrupt, this is retransmit than  SETUP_RETR                          
    {
      SPI_RW_Reg(FLUSH_TX,0);
      SPI_Write_Buf(W_TX_PAYLOAD,tx_buf,TX_PLOAD_WIDTH);	// disable standy-mode
    }
	*/
    SPI_RW_Reg(W_REGISTER+STATUS,sstatus);	// clear RX_DR or TX_DS or MAX_RT interrupt flag
    delay(1000);
  }
}

//**************************************************
void init_io(void)
{
  //digitalWrite(IRQ, 0);
  digitalWrite(CE, 0);			// chip enable
  digitalWrite(CSN, 1);         // Spi disable	
}

//**************************************************
unsigned char SPI_Read(unsigned char reg)
{
  unsigned char reg_val;

  digitalWrite(CSN, 0);                // CSN low, initialize SPI communication...
  SPI_RW(reg);                         // Select register to read from..
  reg_val = SPI_RW(0);                 // ..then read register value
  digitalWrite(CSN, 1);                // CSN high, terminate SPI communication

  return(reg_val);                     // return register value
}

//**************************************************
unsigned char SPI_RW(unsigned char Byte)
{
  return SPI.transfer(Byte);
}

//**************************************************
unsigned char SPI_Read_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes)
{
  unsigned char sstatus,i;

  digitalWrite(CSN, 0);		// Set CSN low, init SPI tranaction
  sstatus = SPI_RW(reg);	// Select register to write to and read status unsigned char

  for(i=0;i<bytes;i++)
  {
    pBuf[i] = SPI_RW(0);	// Perform SPI_RW to read unsigned char from nRF24L01
  }

  digitalWrite(CSN, 1);		// Set CSN high again

  return(sstatus);			// return nRF24L01 status unsigned char
}

//**************************************************
unsigned char SPI_Write_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes)
{
  unsigned char sstatus,i;

  digitalWrite(CSN, 0);		// Set CSN low, init SPI tranaction
  sstatus = SPI_RW(reg);	// Select register to write to and read status unsigned char
  for(i=0;i<bytes; i++)		// then write all unsigned char in buffer(*pBuf)
  {
    SPI_RW(*pBuf++);
  }
  digitalWrite(CSN, 1);		// Set CSN high again
  return(sstatus);			// return nRF24L01 status unsigned char
}

//**************************************************
unsigned char SPI_RW_Reg(unsigned char reg, unsigned char value)
{
  unsigned char status;

  digitalWrite(CSN, 0);		// CSN low, init SPI transaction
  SPI_RW(reg);				// select register
  SPI_RW(value);			// ..and write value to it..
  digitalWrite(CSN, 1);		// CSN high again

  return(status);			// return nRF24L01 status unsigned char
}

//**************************************************
void TX_Mode(void)
{
  digitalWrite(CE, 0);

  SPI_Write_Buf(W_REGISTER + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);	// Writes TX_Address to nRF24L01
  SPI_Write_Buf(W_REGISTER + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);	// RX_Addr0 same as TX_Adr for Auto.Ack

  SPI_RW_Reg(W_REGISTER + EN_AA, 0x01);			// Enable auto acknowledgement data pipe 0
  SPI_RW_Reg(W_REGISTER + EN_RXADDR, 0x01);		// Enable data pipe 0
  SPI_RW_Reg(W_REGISTER + SETUP_RETR, 0x1a);	// auto retransmit delay = 500us, retransmit count = 10
  SPI_RW_Reg(W_REGISTER + RF_CH, 40);			// Select RF channel 40
  SPI_RW_Reg(W_REGISTER + RF_SETUP, 0x07);		// TX_PWR:0dBm, Datarate:1Mbps, LNA:HCURR
  SPI_RW_Reg(W_REGISTER + CONFIG, 0x0e);		// POWER UP, enable 2-byte CRC & PTX as RX/TX control
												// RX_DR, TX_DS & MAX_RT interrupts enabled
  SPI_Write_Buf(W_TX_PAYLOAD,tx_buf,TX_PLOAD_WIDTH);

  digitalWrite(CE, 1);
}