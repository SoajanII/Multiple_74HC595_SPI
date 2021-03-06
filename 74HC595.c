// TM4C and Multiple 74HC595.c (up to 3)
// Original at http://users.ece.utexas.edu/~valvano/arm/LM3Sindex.html
// Made small modification to Valvano's code (modification on delay function)
// SCK (clock) connected to PA2, Frame signal created via PE2-0 , 
// SI (74HC595 receive) connected to PA5. SCLR is reset, 
// and it is connected to 3.3V. QH is floating.
// Remember to connect grounds of power supply and TM4C!

#define GPIO_PORTE_DATA_R       (*((volatile unsigned long *)0x400243FC)) // bits 7-0
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
	
#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
#define GPIO_PORTA_DIR_R        (*((volatile unsigned long *)0x40004400))
#define GPIO_PORTA_DATA_R       (*((volatile unsigned long *)0x400043FC))
#define SSI0_CR0_R              (*((volatile unsigned long *)0x40008000))
#define SSI0_CR1_R              (*((volatile unsigned long *)0x40008004))
#define SSI0_DR_R               (*((volatile unsigned long *)0x40008008))
#define SSI0_SR_R               (*((volatile unsigned long *)0x4000800C))
#define SSI0_CPSR_R             (*((volatile unsigned long *)0x40008010))
#define SSI_CR0_SCR_M           0x0000FF00  // SSI Serial Clock Rate
#define SSI_CR0_SPH             0x00000080  // SSI Serial Clock Phase
#define SSI_CR0_SPO             0x00000040  // SSI Serial Clock Polarity
#define SSI_CR0_FRF_M           0x00000030  // SSI Frame Format Select
#define SSI_CR0_FRF_MOTO        0x00000000  // Freescale SPI Frame Format
#define SSI_CR0_DSS_M           0x0000000F  // SSI Data Size Select
#define SSI_CR0_DSS_8           0x00000007  // 8-bit data
#define SSI_CR1_MS              0x00000004  // SSI Master/Slave Select
#define SSI_CR1_SSE             0x00000002  // SSI Synchronous Serial Port
                                            // Enable
#define SSI_SR_RNE              0x00000004  // SSI Receive FIFO Not Empty
#define SSI_SR_TNF              0x00000002  // SSI Transmit FIFO Not Full
#define SSI_CPSR_CPSDVSR_M      0x000000FF  // SSI Clock Prescale Divisor
#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC1_SSI0       0x00000010  // SSI0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

void Port_Init(void){
  volatile unsigned long delay;
	//setting up SPI0 pins located at PORTA
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_SSI0;  // activate SSI0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // activate port A
  delay = SYSCTL_RCGC2_R;               // allow time to finish activating
  GPIO_PORTA_AFSEL_R |= 0x24;           // enable alt funct on PA2,5 ; no need for PA4 (receiver) since we don't use it
																				// also we will not use PA3 (normaly Fss)
																				// instead PE0,1,2 will be used as manual Fss - GPIO style
	
  GPIO_PORTA_DEN_R |= 0x2C;             // enable digital I/O on PA2,3,5
	
	//enabling PORTE0,1,2 as output GPIO pins 
  SYSCTL_RCGC2_R |= 0x10;          // activate Port E
  delay = SYSCTL_RCGC2_R;          // allow time for clock to stabilize
                                   // no need to unlock PE1-0
  GPIO_PORTE_AMSEL_R &= ~0x07;     // disable analog function on PE2-0
  GPIO_PORTE_PCTL_R &= ~0x00000FFF;// configure PE2-0 as GPIO
  GPIO_PORTE_DIR_R |= 0x07;        // make PE2-0 out
  GPIO_PORTE_AFSEL_R &= ~0x07;     // disable alt funct on PE2-0
  GPIO_PORTE_DEN_R |= 0x07;        // enable digital I/O on PE2-0   
	
	//setting up SPI
  SSI0_CR1_R &= ~SSI_CR1_SSE;           // disable SSI
  SSI0_CR1_R &= ~SSI_CR1_MS;            // master mode (default setting)
                                        // clock divider for 3 MHz SSIClk
  SSI0_CPSR_R = (SSI0_CPSR_R&~SSI_CPSR_CPSDVSR_M)+2;
  SSI0_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (3 Mbps data rate) (default setting)
                  SSI_CR0_SPH |         // SPH = 0 (default setting)
                  SSI_CR0_SPO);         // SPO = 0 (default setting)
                                        // FRF = Freescale format (default setting)
  SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_FRF_M)+SSI_CR0_FRF_MOTO;
                                        // DSS = 8-bit data
  SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_DSS_M)+SSI_CR0_DSS_8;
  SSI0_CR1_R |= SSI_CR1_SSE;            // enable SSI
}

void delay(unsigned long ten_ns){
  unsigned long count;
 
  while(ten_ns > 0 ) {
    count = 31;
// originally count was 400000, which took 130000 ns to complete
// later we change it to 400000*10/130000=31 that it takes ~9-10 ns
    while (count > 0) {
      count--;
    }
    ten_ns--;
  }
}

void Port_Out(unsigned long alias_PORTE_74HC595, unsigned short value)
{
	GPIO_PORTE_DATA_R |= alias_PORTE_74HC595;
  while((SSI0_SR_R&SSI_SR_TNF)==0){};// wait until room in FIFO
  SSI0_DR_R = value;                  // data out
  while((SSI0_SR_R&SSI_SR_RNE)==0){};// wait until response
  delay(2);
	GPIO_PORTE_DATA_R &= ~(alias_PORTE_74HC595);		
}

int main(void){
  unsigned char i=0;
  Port_Init();
	GPIO_PORTE_DATA_R &= ~0x07; //clear bits of PE2-0
	
  while(1){
    Port_Out(0x01, i);    
		Port_Out(0x02, i+39);  
    i = i + 1;
    delay(10000);                        // 100 msec delay
  }
}
