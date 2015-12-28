#ifndef USB_HARDWARE_PROFILE_H
#define USB_HARDWARE_PROFILE_H


//#define CLOCK_FREQ 48000000
//#define BAUDCLOCK_FREQ 12000000 // (48000000 /4) required for baud rate calculations
//#define UART_BAUD_setup(x)  SPBRG = x & 0xFFu; SPBRGH = (x >> 8) & 0xFFu
//#define CDC_FLUSH_MS 4 // how many ms timeout before cdc in to host is sent

#define USB_VID (0x2a19)  // Numato Labs
#define USB_PID (0x5446)  // Low Speed I/O - TOFE Expansion Board
#define USB_DEV (0x0001)

#define USB_NUM_CONFIGURATIONS          1u
#define USB_NUM_INTERFACES              4u
#define USB_NUM_ENDPOINTS               6u


#define MAX_EPNUM_USED                  4u

#define USB_BUS_POWERED 0
#define USB_INTERNAL_TRANSCIEVER 1
#define USB_INTERNAL_PULLUPS 1
#define USB_INTERNAL_VREG 1
#define USB_FULL_SPEED_DEVICE 1

/* PingPong Buffer Mode
 * Valid values
 * 0 - No PingPong Buffers
 * 1 - PingPong on EP0
 * 2 - PingPong on all EP
 * 3 - PingPong on all except EP0
 */
#define USB_PP_BUF_MODE 0
#define USB_EP0_BUFFER_SIZE 8u
#define CDC_NOTICE_BUFFER_SIZE 10u

/* Low Power Request
 * Optional user supplied subroutine to set the circuit
 * into low power mode during usb suspend.
 * Probably needed when bus powered as only 2.5mA should
 * be drawn from bus i suspend mode */
//#define usb_low_power_request() Nop()

#define CDC_BUFFER_SIZE 32u

#define CLOCK_FREQ 48000000
#define BAUDCLOCK_FREQ 12000000 // (48000000 /4) required for baud rate calculations
#define UART_BAUD_setup(x)  SPBRG = x & 0xFFu; SPBRGH = (x >> 8) & 0xFFu
#define CDC_FLUSH_MS 4 // how many ms timeout before cdc in to host is sent

#define USB_INTERRUPTS //use interrupts instead of polling


#endif
