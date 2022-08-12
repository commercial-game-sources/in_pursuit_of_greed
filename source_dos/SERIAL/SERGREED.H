/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Serial Communication Driver for Greed                                   */
/* Copyright (C) 1995 by Channel 7                                         */
/*                                                                         */
/* written by Robert Morgan                                                */
/*                                                                         */
/***************************************************************************/


/**** CONSTANTS ****/

#define CLI() _disable()
#define STI() _enable()

#define REG_TX                               uart
#define REG_RX                               uart
#define REG_INT_EN                           (uart+0x01)
#define   IER_RX_DATA_READY                  0x01
#define   IER_TX_HOLDING_REGISTER_EMPTY      0x02
#define   IER_LINE_STATUS                    0x04
#define   IER_MODEM_STATUS                   0x08
#define REG_INT_ID                           (uart+0x02)
#define   IIR_NOPEND                         0x01
#define   IIR_MSTAT                          0x00
#define   IIR_TX                             0x02
#define   IIR_RX                             0x04
#define   IIR_LSTAT                          0x06
#define REG_FCONT                            (uart+0x02)
#define   FCR_FIFO_ENABLE                    0x01
#define   FCR_RCVR_FIFO_RESET                0x02
#define   FCR_XMIT_FIFO_RESET                0x04
#define   FCR_RCVR_TRIGGER_LSB               0x40
#define   FCR_RCVR_TRIGGER_MSB               0x80
#define   FCR_TRIGGER_01                     0x00
#define   FCR_TRIGGER_04                     0x40
#define   FCR_TRIGGER_08                     0x80
#define   FCR_TRIGGER_16                     0xC0
#define REG_LCONT                            (uart+0x03)
#define   LCONT_WORD_LENGTH_MASK             0x03
#define   LCONT_WORD_LENGTH_SELECT_0         0x01
#define   LCONT_WORD_LENGTH_SELECT_1         0x02
#define   LCONT_STOP_BITS                    0x04
#define   LCONT_PARITY_MASK                  0x38
#define   LCONT_PARITY_ENABLE                0x08
#define   LCONT_EVEN_PARITY_SELECT           0x10
#define   LCONT_STICK_PARITY                 0x20
#define   LCONT_SET_BREAK                    0x40
#define   LCONT_DLAB                         0x80
#define REG_MCONT                            (uart+0x04)
#define   MCONT_DTR                          0x01
#define   MCONT_RTS                          0x02
#define   MCONT_OUT1                         0x04
#define   MCONT_OUT2                         0x08
#define   MCONT_LOOPBACK                     0x10
#define REG_LSTAT                            (uart+0x05)
#define   LSTAT_DATA_READY                   0x01
#define   LSTAT_OVERRUN_ERROR                0x02
#define   LSTAT_PARITY_ERROR                 0x04
#define   LSTAT_FRAMING_ERROR                0x08
#define   LSTAT_BREAK_DETECT                 0x10
#define   LSTAT_THRE                         0x20
#define REG_MSTAT                            (uart+0x06)
#define   MSTAT_DELTA_CTS                    0x01
#define   MSTAT_DELTA_DSR                    0x02
#define   MSTAT_TERI                         0x04
#define   MSTAT_DELTA_CD                     0x08
#define   MSTAT_CTS                          0x10
#define   MSTAT_DSR                          0x20
#define   MSTAT_RI                           0x40
#define   MSTAT_CD                           0x80

#define PARITY_NONE                          0x00
#define PARITY_ODD                           0x01
#define PARITY_EVEN                          0x02

#define QUESIZE                              4095

#define outbyte(r,b)                         (void)outp(r,b)
#define outword(r,w)                         (void)outpw(r,w)
#define inbyte(r)                            inp(r)

#define MAXPACKET                            128
#define FRAMECHAR                            0x70
#define FRAMETERM                            0x00
#define MAXMODEMSTRLEN                       80
#define MAXSPEEDSTR                          "9600"
#define SERIALBAUD                           9600


/**** TYPES ****/

typedef unsigned char byte;

typedef struct
{
 long head, tail;    // bytes are put on head and pulled from tail
 byte data[QUESIZE+1];
 } que_t;


typedef struct
{
 que_t in;
 que_t out;
 short uart, uarttype, irqintnum;
 short intseg, intofs;
 short rsent, rreceived;
 short psent, preceived;
 } ques_t;


typedef enum {false,true} boolean;


/**** VARIABLES ****/

extern char    packet[MAXPACKET];
extern int     packetlen, dupnum, badpackets, uart, totalpackets;
extern boolean modemactive;
extern char    shutdown[MAXMODEMSTRLEN];
extern ques_t  que;


/**** FUNCTIONS ****/

void interrupt ISR_8250(void);
void           COM_InitPort(void);
void           COM_ShutdownPort(void);
int            COM_ReadByte(void);
void           COM_WriteBuffer(char *buffer, unsigned int count);
int            COM_ReadPacket(void);
void           COM_WritePacket(char *buffer, int len);
void           COM_BaudSet(long baudrate);
void           COM_ParmSet(int parity,int stop,int databits);
void           MS_Error(char *error,...);
void           MOD_Command(char *str);
void           MOD_Dial(void);
void           MOD_Answer(void);
void           MOD_Init(void);


