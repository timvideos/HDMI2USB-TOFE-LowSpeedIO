/*
This work is licensed under the Creative Commons Attribution 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/
or send a letter to
        Creative Commons,
        171 Second Street,
        Suite 300,
        San Francisco,
        California,
        94105,
        USA.
 */

// JTR V0.1a
// JTR v0.1b
// JTR v0.1c	// tidy up added code line code handler to set the SPBRG based on line coding.
// JTR V0.2a   // 26th Jan 2012

#include "usb_stack_globals.h"    // USB stack only defines Not function related.

#include <string.h>

enum stopbits {
    one = 0, oneandahalf = 1, two = 2
};

enum parity {
    none = 0, odd = 1, even = 2, mark = 3, space = 4
};
const char parity_str[] = {'N', 'O', 'E', 'M', 'S'};

struct cdc_LineCodeing {
    unsigned long int dwDTERate;
    enum stopbits bCharFormat;
    enum parity bParityType;
    BYTE bDataBits;
};

struct cdc_LineCodeing cdcdata_LineCodeing;

#pragma udata usb_data
BYTE cdcdata_ACMInBuffer[CDC_NOTICE_BUFFER_SIZE]; //JTR NEWLY defined NOTICE BUFFER SIZE and increased from 8 to 10 bytes in usb_config.h

#pragma udata usb_data3
BYTE cdcdata_InBufferA[CDC_BUFFER_SIZE];
BYTE cdcdata_InBufferB[CDC_BUFFER_SIZE];
BYTE cdcdata_OutBufferA[CDC_BUFFER_SIZE];
BYTE cdcdata_OutBufferB[CDC_BUFFER_SIZE];

#pragma udata

struct _cdc_ControlLineState cdcdata_ControlLineState;

BYTE cdcdata_InLen; // total cdc In length
volatile BYTE cdcdata_OutLen; // total cdc out length
BYTE cdcdata_IsInBufferA;
BYTE cdcdata_IsOutBufferA;
BYTE *cdcdata_InPtr;
BYTE *cdcdata_OutPtr;
BYTE cdcdata_LineStateUpdated = 0;
BYTE cdcdata_TimeoutCount = 0;
BYTE cdcdata_ZLPPending = 0;
BYTE cdcdata_Lock = 0;

BDentry *cdcdata_Outbdp, *cdcdata_Inbdp;
BYTE CDCFunctionError;

volatile BYTE cdcdata_TRFState; // JTR don't see that it is really volatile in current context may be in future.

void cdc_init(void) {

    // JTR The function usb_init() is now called from main.c prior to anything else belonging to the CDC CLASS
    // If we have multiple functions we want the USB initialization to be in only one consistant place.
    // The sort of things we would do in InitCDC would be to setup I/O pins and the HARDWARE UART so it
    // is not transmitting junk between a RESET and the device being enumerated. Hardware CTS/RTS
    // would also be setup here if being used.

    cdcdata_LineCodeing.dwDTERate = 115200;
    cdcdata_LineCodeing.bCharFormat = one;
    cdcdata_LineCodeing.bParityType = none;
    cdcdata_LineCodeing.bDataBits = 8;

    cdcdata_ControlLineState.DTR = 0;
    cdcdata_ControlLineState.RTS = 0;
    usb_register_class_setup_handler(cdc_setup);
}

void user_configured_init(void) {
    // JTR NEW FUNCTION
    // After the device is enumerated and configured then we set up non EP0 endpoints.
    // We only enable the endpoints we are using, not all of them.
    // Prior to this they are held in a disarmed state.

    // This function belongs to the current USB function and IS NOT generic. This is CLASS specific
    // and will vary from implementation to implementation.

    usb_unset_in_handler(1);
    usb_unset_in_handler(2);
    usb_unset_out_handler(2);

    USB_UEP1 = USB_EP_IN;
    USB_UEP2 = USB_EP_INOUT;

    /* Configure buffer descriptors */
#if USB_PP_BUF_MODE == 0
    // JTR Setup CDC LINE_NOTICE EP (Interrupt IN)
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdcdata_ACMInBuffer;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS + DTSEN; // Set DTS => First packet inverts, ie. is Data0
#else
    // TODO: Implement Ping-Pong buffering setup.
#error "PP Mode not implemented yet"
#endif

    usb_register_class_setup_handler(cdc_setup);
    cdcdata_TRFState = 0;
    cdcdata_Outbdp = &usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)];
    cdcdata_Inbdp = &usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)];

    cdcdata_IsInBufferA = 0xFF;
    cdcdata_InPtr = cdcdata_InBufferA;
    cdcdata_InLen = 0;
    cdcdata_Inbdp->BDADDR = &cdcdata_InBufferA[0];
    cdcdata_Inbdp->BDCNT = 0;
    cdcdata_Inbdp->BDSTAT = DTS + DTSEN;

    cdcdata_OutLen = 0;
    cdcdata_IsOutBufferA = 0xFF;
    cdcdata_OutPtr = cdcdata_OutBufferA;
    cdcdata_Outbdp->BDCNT = CDC_BUFFER_SIZE;
    cdcdata_Outbdp->BDADDR = &cdcdata_OutBufferA[0];
    cdcdata_Outbdp->BDSTAT = UOWN + DTSEN;
}

void cdc_setup(void) {
    BYTE *packet;
    size_t reply_len;
    packet = EP0_Outbdp->BDADDR;

    switch (packet[USB_bmRequestType] & (USB_bmRequestType_TypeMask | USB_bmRequestType_RecipientMask)) {
        case (USB_bmRequestType_Class | USB_bmRequestType_Interface):
            switch (packet[USB_bRequest]) {

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_SEND_ENCAPSULATED_COMMAND:
                    usb_ack_dat1(0);
                    break;

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_GET_ENCAPSULATED_RESPONSE:
                    //usb_ack_zero(rbdp);
                    usb_ack_dat1(0);
                    break;

                case CDC_SET_COMM_FEATURE: // Optional
                case CDC_GET_COMM_FEATURE: // Optional
                case CDC_CLEAR_COMM_FEATURE: // Optional
                    usb_RequestError(); // Not advertised in ACM functional descriptor
                    break;

                case CDC_SET_LINE_CODING: // Optional, strongly recomended
                    usb_set_out_handler(0, cdc_set_line_coding_data); // Register out handler function
                    break;

                case CDC_GET_LINE_CODING: // Optional, strongly recomended
                    // JTR reply length (7) is always going to be less than minimum EP0 size (8)

                    reply_len = *((unsigned int *) &packet[USB_wLength]);
                    if (sizeof (struct cdc_LineCodeing) < reply_len) {
                        reply_len = sizeof (struct cdc_LineCodeing);
                    }
                    memcpy(EP0_Inbdp->BDADDR, (const void *) &cdcdata_LineCodeing, reply_len);
                    usb_ack_dat1(reply_len); // JTR common addition for STD and CLASS ACK
                    usb_set_in_handler(0, cdc_get_line_coding);
                    break;

                case CDC_SET_CONTROL_LINE_STATE: // Optional
                    cdcdata_ControlLineState = *((struct _cdc_ControlLineState *) &packet[USB_wValue]);
                    usb_set_in_handler(0, cdc_set_control_line_state_status); // JTR why bother?
                    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK
                    cdcdata_LineStateUpdated = 1;
                    break;

                case CDC_SEND_BREAK: // Optional
                default:
                    usb_RequestError();
            }
            break;
        default:
            usb_RequestError();
    }
}

void cdc_get_line_coding(void) {
    usb_unset_in_handler(0); // Unregister IN handler;
}

void cdc_set_line_coding_data(void) { // JTR handling an OUT token In the CDC stack this is the only function that handles an OUT data stage.
    unsigned long dwBaud, dwBaudrem;

    memcpy(&cdcdata_LineCodeing, (const void *) EP0_Outbdp->BDADDR, sizeof (struct cdc_LineCodeing));

    dwBaud = BAUDCLOCK_FREQ / cdcdata_LineCodeing.dwDTERate;
    dwBaudrem = BAUDCLOCK_FREQ % cdcdata_LineCodeing.dwDTERate;
    if (cdcdata_LineCodeing.dwDTERate > (dwBaudrem << 1))
        dwBaud--;

    UART_BAUD_setup(dwBaud);

    usb_unset_out_handler(0); // Unregister OUT handler; JTR serious bug fix in macro!
    usb_set_in_handler(0, cdc_set_line_coding_status); // JTR why bother?
    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK

    // JTR This part of the USB-CDC stack is worth highlighting
    // This is the only place that we have an OUT DATA packet on
    // EP0. At this point it has been completed. This stack unlike
    // the microchip stack does not have a common IN or OUT data
    // packet complete tail and therefore it is the responsibility
    // of each section to ensure that EP0 is set-up correctly for
    // the next setup packet.


    //  Force EP0 OUT to the DAT0 state
    //  after we have all our data packets.
    EP0_Outbdp->BDCNT = USB_EP0_BUFFER_SIZE;
    EP0_Outbdp->BDSTAT = UOWN | DTSEN;
}

void cdc_set_line_coding_status(void) {
    usb_unset_in_handler(0);
}

void cdc_set_control_line_state_status(void) {
    usb_unset_in_handler(0);
}

/*****************************************************************************/
void cdc_wait_out_ready() // JTR2 added reduced overhead
{
    while ((cdcdata_Outbdp->BDSTAT & UOWN));
}

/******************************************************************************/

void cdc_wait_in_ready() // JTR2 added reduced overhead
{
    while ((cdcdata_Inbdp->BDSTAT & UOWN));
}//end cdc_wait_in_ready

/******************************************************************************/
BYTE cdc_out_ready(void) {

    return !(cdcdata_Outbdp->BDSTAT & UOWN); // Do we have a packet from host?
}

/******************************************************************************/
BYTE cdc_in_ready(void) {

    return !(cdcdata_Inbdp->BDSTAT & UOWN); // Is the CDC In buffer ready?
}

/******************************************************************************/
BYTE cdc_getda(void) {

    CDCFunctionError = 0;

    cdc_wait_out_ready();

    if ((cdcdata_IsOutBufferA & 1)) {
        cdcdata_OutPtr = &cdcdata_OutBufferA[0];
        cdcdata_Outbdp->BDADDR = &cdcdata_OutBufferB[0];
    } else {
        cdcdata_OutPtr = &cdcdata_OutBufferB[0];
        cdcdata_Outbdp->BDADDR = &cdcdata_OutBufferA[0];
    }
    cdcdata_IsOutBufferA ^= 0xFF;
    cdcdata_OutLen = cdcdata_Outbdp->BDCNT;
    cdcdata_Outbdp->BDCNT = CDC_BUFFER_SIZE;
    cdcdata_Outbdp->BDSTAT = ((cdcdata_Outbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
#ifndef USB_INTERRUPTS
    usb_handler();
#endif
    return cdcdata_OutLen;
}//end getCDC_Out_ArmNext

BYTE putda_cdc(BYTE count) {

    //    CDCFunctionError = 0;
    //    cdc_wait_in_ready();
    while ((cdcdata_Inbdp->BDSTAT & UOWN));
    if (cdcdata_IsInBufferA) {
        cdcdata_Inbdp->BDADDR = cdcdata_InBufferA;
        cdcdata_InPtr = cdcdata_InBufferB;
    } else {
        cdcdata_Inbdp->BDADDR = cdcdata_InBufferB;
        cdcdata_InPtr = cdcdata_InBufferA;
    }
    cdcdata_Inbdp->BDCNT = count;
    cdcdata_Inbdp->BDSTAT = ((cdcdata_Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    cdcdata_IsInBufferA ^= 0xFF;
#ifndef USB_INTERRUPTS
    usb_handler();
#endif
    return 0; //CDCFunctionError;
}

void SendZLP(void) {
    putda_cdc(0);
}

/******************************************************************************/
void cdc_flush_in_now(void) {
    if (cdcdata_InLen > 0) {
        while (!cdc_in_ready());
        putda_cdc(cdcdata_InLen);
        if (cdcdata_InLen == CDC_BUFFER_SIZE) {
            cdcdata_ZLPPending = 1;
        } else {
            cdcdata_ZLPPending = 0;
        }
        cdcdata_InLen = 0;
        cdcdata_TimeoutCount = 0;
    }
}

/******************************************************************************/
void cdc_flush_on_timeout(void) {
    if (cdcdata_TimeoutCount >= CDC_FLUSH_MS) { // For timeout value see: cdc_config.h -> [hardware] -> CDC_FLUSH_MS
        if (cdcdata_InLen > 0) {
            if ((cdcdata_Lock == 0) && cdc_in_ready()) {
                putda_cdc(cdcdata_InLen);
                if (cdcdata_InLen == CDC_BUFFER_SIZE) {
                    cdcdata_ZLPPending = 1;
                } else {
                    cdcdata_ZLPPending = 0;
                }
                cdcdata_InLen = 0;
                cdcdata_TimeoutCount = 0;
            }
        } else if (cdcdata_ZLPPending) {
            putda_cdc(0);
            cdcdata_ZLPPending = 0;
            cdcdata_TimeoutCount = 0;
        }
    } else {
        cdcdata_TimeoutCount++;
    }
}

/******************************************************************************/
void cdc_putc(BYTE c) {
    cdcdata_Lock = 1; // Stops cdc_flush_on_timeout() from sending per chance it is on interrupts.
    *cdcdata_InPtr = c;
    cdcdata_InPtr++;
    cdcdata_InLen++;
    cdcdata_ZLPPending = 0;
    if (cdcdata_InLen == CDC_BUFFER_SIZE) {
        putda_cdc(cdcdata_InLen); // This will stall tranfers if both buffers are full then return when a buffer is available.
        cdcdata_InLen = 0;
        cdcdata_ZLPPending = 1; // timeout handled in the SOF handler below.
    }
    cdcdata_Lock = 0;
    cdcdata_TimeoutCount = 0; //setup timer to throw data if the buffer doesn't fill
}

/******************************************************************************/
// Waits for a byte to be available and returns that byte as a
// function return value. The byte is removed from the CDC OUT queue.
// No return count is required as this function always returns one byte.

BYTE cdc_getc(void) { // Must be used only in double buffer mode.
    BYTE c = 0;

    if (cdcdata_OutLen == 0) {
        do {
            cdcdata_OutLen = cdc_getda();
        } while (cdcdata_OutLen == 0); // Skip any ZLP
    }
    c = *cdcdata_OutPtr;
    cdcdata_OutPtr++;
    cdcdata_OutLen--;
    return c;
}

/******************************************************************************/
// Checks to see if there is a byte available in the CDC buffer.
// If so, it returns that byte at the dereferenced pointer *C
// and the function returns a count of 1. The byte is effectively
// removed from the queue.
// IF no byte is available function returns immediately with a count of zero.

BYTE cdc_poll_getc(BYTE * c) { // Must be used only in double buffer mode.
    if (cdcdata_OutLen) { // Do we have a byte waiting?
        *c = *cdcdata_OutPtr; // pass it on and adjust cdcdata_OutPtr and count
        cdcdata_OutPtr++;
        cdcdata_OutLen--;
        return 1; // Return byte count, always one.
    }
    if (cdc_out_ready()) { // No byte in queue check for new arrivals.
        cdcdata_OutLen = cdc_getda();
        if (cdcdata_OutLen) {
            *c = *cdcdata_OutPtr;
            cdcdata_OutPtr++;
            cdcdata_OutLen--;
            return 1;
        }
    }
    return 0;
}

/******************************************************************************/
// Checks (PEEKS) to see if there is a byte available in the CDC buffer.
// If so, it returns that byte at the dereferenced pointer *C
// and the function returns a count of 1. The byte however is NOT
// removed from the queue and can still be read with the cdc_poll_getc()
// and getc_cdc() functions that will remove it from the queue.
// IF no byte is available function returns immediately with a count of zero.

BYTE cdc_peek_getc(BYTE * c) { // Must be used only in double buffer mode.
    if (cdcdata_OutLen) {
        *c = *cdcdata_OutPtr;
        return 1;
    }
    if (cdc_out_ready()) {
        cdcdata_OutLen = cdc_getda();
        if (cdcdata_OutLen) {
            *c = *cdcdata_OutPtr;
            return 1;
        }
    }
    return 0;
}

