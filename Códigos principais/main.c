
#include "msp430f1611.h"

  void Config_LCD();
  void CLR_LCD();
  void envia_dado();
  void envia_comando();
  void mostrardisplay();
  void config_teclado();
  void stop();             // � a fun��o que ir� sinalizar que o processo terminou
  void salvaLetra();
  void LerTemp();
  unsigned char rd_tec();
  unsigned char tecla;
  unsigned int Temp;
  int digito;          // Ir� indicar qual posi��o do display ser� alterada
  int sensor;
  int segundo;
  int fornoquente;

  char minutos[]={'1','2','0',' ','M','i','n','u','t','o','s'}; //Vetor que ir� armazenar o tempo restante
  char temperatura[]= {'2','0','6',' ','G','r','a','u','s'};    //Vetor que ir� armazenar a temperatura desejada
  char fim []= {'A','C','A','B','O','U','!','!','!'};           //Vetor que ir� aparecer quando acabar o processo

void mostrardisplay(void)       // Essa fun��o � respons�vel por mostrar o tempo restante na linha de cima do display
                                // na linha de cima do display e a temperatura escolhida na linha de baixo do display
{
  P5OUT=0x83;                   // Centraliza a primeira linha
  envia_comando();
  for(int i=0;i<11;i++)         // Escreve o vetor do tempo na primeira linha
     {P5OUT=minutos[i];envia_dado();}
  P5OUT=0xC3;                   // Centraliza a segunda linha
  envia_comando();
  for(int i=0;i<9;i++ )         // Escreve o vetor da temperatura na segunda linha
     {P5OUT=temperatura[i];envia_dado();}
  }


 

void hora (void)
{
  if (segundo==0)                      // Checa se passou 1 minuto
     {segundo=60;minutos[2]--;}
  if(minutos[2]<'0')                   // Checa se passou 10 minutos
     {minutos[2]='9';minutos[1]--;}
  if(minutos[0]!='0')                  // Checa se � possivel diminuir o valor da casa de centenas de minutos
    {
     if(minutos[1]<'0')                // Checa se faltam 99 minutos para o fim
       {minutos[1]='9';minutos[0]--;}
    }
   if ((minutos[0]=='0')&&(minutos[1]=='0')&&(minutos[2]=='0'))    // Checa se o tempo acabou
     {stop();}
   if((temperatura[0]>='2')&&(temperatura[1]>='5'))                // Checa se a temperatura desejada est� acima do m�ximo permitido
     {
       temperatura[0]='2';
       temperatura[1]='5';
     }
  }

void stop (void)               // Subrotina de quando o tempo acabar
  {
   TBCTL=TBSSEL_1+MC_0;        // Para o PWM
   CLR_LCD();                  // Prepara o LCD para a escrita de outro vetor
   P5OUT=0x83;
    envia_comando();
    for(int i=0;i<9;i++)
      {P5OUT=fim[i];envia_dado();}
    P4OUT^=0x01;
    __delay_cycles(15000);
    if (rd_tec()=='E')
    {
     digito=0;
     segundo=60;
     fornoquente=0;
     TBCTL=TBSSEL_1+MC_1;
   minutos[0]='0';
   minutos[1]='0';
   minutos[2]='0';
    }
 }

void salvaLetra(int tecla)                   // subrotina que altera os vetores que est�o no display
  {
   switch (tecla)                            // Separa as a��es devido � casos espec�ficos, como
    {
      case 'E':                              // Tecla Enter , significa que o operador acabou de digitar os vetores e pode-se come�ar a contagem
        TACTL=TASSEL_1+MC_1;                 // do tempo do forno
        break;
      case 'A':                              // Tecla Anula, volta uma posi��o no vetor que est� sendo alterado
        digito--;
        break;
      case 'n':                              // Esse caso significa que a tecla pressionada n�o foi identificada
        break;
      default:
        if(digito>5)                         // checa se a posi��o dos vetores a serem alterados n�o foi estourada
          {digito=0;}
        if (digito>2)                        // checa se a casa a ser alterada � do vetor temperatura, caso n�o seja o vetor a ser alterado ser� minutos
           {temperatura[(digito-3)]=tecla;}
        else
           {
            minutos[digito]=tecla;
            }
        digito++;                            // Passa para a pr�xima posi��o dos vetores
        break;
     }
  }

  void LerTemp()
{
  ADC12CTL0 |= ADC12SC;                      // inicia convers�o
  while ((ADC12IFG & BIT0)==0);
        {sensor = ((ADC12MEM0/9)+1);}           // Vari�vel sensor se torna um valor em graus da temperatura real do forno
    
  Temp=(temperatura[0]-0x30)*100+(temperatura[1]-0x30)*10+(temperatura[2]-0x30); // Carrega o novo valor de temperatura
  
  if(sensor<Temp)                            // Analisa a temperatura do forno e a compara com o preset feito pelo operador
    {TBCCR1=270;}                          // Se temperatura do forno � menor do que o preset o ciclo de trabalho do PWM � aprox. 90%
  else
    {TBCCR1=0;fornoquente=1;}                     // Se temperatura do forno � maior do que o preset o ciclo de trabalho do PWM � aprox. 10%
}


  #pragma vector=TIMERA0_VECTOR
    __interrupt void TimerA (void)
    {
      if (fornoquente==1)
         {segundo--;hora();}
      P4OUT^=0x01;
    }

  #pragma vector=PORT2_VECTOR                // Endere�o da interrup��o do teclado
    __interrupt void Porta2 (void)
  {
   tecla=rd_tec();                           // Carrega o valor da tecla pressionada no caracter ''tecla''
   salvaLetra(tecla);                        // Subrotina que realiza as opera��es necess�rias para cada tecla
   P2IFG=0;
   __delay_cycles(200000);                   // Delay que previne repiques e o tempo de pressionamento
  }

void main( void )
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer to prevent time out reset
   Config_LCD();
   config_teclado();
   
   P4DIR|=BIT1;                             // Configura a porta 4 para ser a sa�da do PWM
   P4SEL=BIT1;                               // BIT1 da porta 4 ligada ao TimerB1 (Sa�da do PWM)
    
   P4DIR|=BIT0;                              // Sa�da LED ser� na porta P2.1
   
   TACCTL0=CCIE;                            // Habilita a interrup��o do bloco TACCR0 ao chegar em 32768
   TACCR0=32768;                             // Tempo de 1 segundo
   
   TBCCTL1=OUTMOD_7;                         // Define o modo de opera��o dos timers 
   TBCTL=TBSSEL_1+MC_1;                      //Aciona Timer B
   
   P6DIR=0x00;                               // Escolhe o pino de leitura da temperatura
   P6SEL |= 0x01;                            // habilita o canal A0 para convers�o digital
   
   ADC12CTL0 = ADC12ON+SHT0_2;               // liga o ADC12, e define tempo de amostragem,al�m de habilitar a convers�o
   ADC12CTL1 = SHP;                          // Usar sampling timer
   ADC12MCTL0 = SREF_0;             
   ADC12CTL0|=ENC
   TBCCR0=300;
   digito=0;
   segundo=60;
   fornoquente=0;
   _BIS_SR(GIE);                             // habilita as interrup��es
   while(1)
  {
    mostrardisplay();
    LerTemp();
  }
}