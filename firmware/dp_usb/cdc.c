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

struct cdc_LineCodeing cdcdata_LineCodeing[2];

#pragma udata usb_data
BYTE cdcdata0_ACMInBuffer[CDC_NOTICE_BUFFER_SIZE];
BYTE cdcdata1_ACMInBuffer[CDC_NOTICE_BUFFER_SIZE];

#pragma udata usb_data3
BYTE cdcdata0_InBufferA[CDC_BUFFER_SIZE];
BYTE cdcdata0_InBufferB[CDC_BUFFER_SIZE];
BYTE cdcdata0_OutBufferA[CDC_BUFFER_SIZE];
BYTE cdcdata0_OutBufferB[CDC_BUFFER_SIZE];

BYTE cdcdata1_InBufferA[CDC_BUFFER_SIZE];
BYTE cdcdata1_InBufferB[CDC_BUFFER_SIZE];
BYTE cdcdata1_OutBufferA[CDC_BUFFER_SIZE];
BYTE cdcdata1_OutBufferB[CDC_BUFFER_SIZE];

#pragma udata
struct _cdc_ControlLineState cdcdata_ControlLineState[2];
BYTE cdcdata_InLen[2]; // total cdc In length
volatile BYTE cdcdata_OutLen[2]; // total cdc out length
BYTE cdcdata_IsInBufferA[2];
BYTE cdcdata_IsOutBufferA[2];
BYTE *cdcdata_InPtr[2];
BYTE *cdcdata_OutPtr[2];
BYTE cdcdata_LineStateUpdated[2] = {0, 0};
BYTE cdcdata_TimeoutCount[2] = {0, 0};
BYTE cdcdata_ZLPPending[2] = {0, 0};
BYTE cdcdata_Lock[2] = {0, 0};
BDentry *cdcdata_Outbdp[2], *cdcdata_Inbdp[2];
volatile BYTE cdcdata_TRFState[2]; // JTR don't see that it is really volatile in current context may be in future.

#pragma code
BYTE CDCFunctionError;

void cdc_init_port(BYTE port) {
    cdcdata_LineCodeing[port].dwDTERate = 115200;
    cdcdata_LineCodeing[port].bCharFormat = one;
    cdcdata_LineCodeing[port].bParityType = none;
    cdcdata_LineCodeing[port].bDataBits = 8;
    cdcdata_ControlLineState[port].DTR = 0;
    cdcdata_ControlLineState[port].RTS = 0;
}

void cdc_init() {
    // JTR The function usb_init() is now called from main.c prior to anything else belonging to the CDC CLASS
    // If we have multiple functions we want the USB initialization to be in only one consistant place.
    // The sort of things we would do in InitCDC would be to setup I/O pins and the HARDWARE UART so it
    // is not transmitting junk between a RESET and the device being enumerated. Hardware CTS/RTS
    // would also be setup here if being used.

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

    usb_unset_in_handler(3);
    usb_unset_in_handler(4);
    usb_unset_out_handler(4);

    USB_UEP1 = USB_EP_IN;
    USB_UEP2 = USB_EP_INOUT;
    USB_UEP3 = USB_EP_IN;
    USB_UEP4 = USB_EP_INOUT;

    /* Configure buffer descriptors */
#if USB_PP_BUF_MODE == 0
    // JTR Setup CDC LINE_NOTICE EP (Interrupt IN)
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdcdata0_ACMInBuffer;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS + DTSEN; // Set DTS => First packet inverts, ie. is Data0

    // JTR Setup CDC LINE_NOTICE EP (Interrupt IN)
    usb_bdt[USB_CALC_BD(3, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(3, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdcdata1_ACMInBuffer;
    usb_bdt[USB_CALC_BD(3, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS + DTSEN; // Set DTS => First packet inverts, ie. is Data0
#else
    // TODO: Implement Ping-Pong buffering setup.
#error "PP Mode not implemented yet"
#endif
    cdc_init_port(0);
    cdcdata_TRFState[0] = 0;
    cdcdata_LineStateUpdated[0] = 0;
    cdcdata_TimeoutCount[0] = 0;
    cdcdata_ZLPPending[0] = 0;
    cdcdata_Lock[0] = 0;

    cdcdata_Outbdp[0] = &usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)];
    cdcdata_Inbdp[0] = &usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)];

    cdcdata_InLen[0] = 0;
    cdcdata_IsInBufferA[0] = 0xFF;
    cdcdata_InPtr[0] = cdcdata0_InBufferA;
    cdcdata_Inbdp[0]->BDADDR = &cdcdata0_InBufferA[0];
    cdcdata_Inbdp[0]->BDCNT = 0;
    cdcdata_Inbdp[0]->BDSTAT = DTS + DTSEN;

    cdcdata_OutLen[0] = 0;
    cdcdata_IsOutBufferA[0] = 0xFF;
    cdcdata_OutPtr[0] = cdcdata0_OutBufferA;
    cdcdata_Outbdp[0]->BDCNT = CDC_BUFFER_SIZE;
    cdcdata_Outbdp[0]->BDADDR = &cdcdata0_OutBufferA[0];
    cdcdata_Outbdp[0]->BDSTAT = UOWN + DTSEN;

    cdc_init_port(1);
    cdcdata_TRFState[1] = 0;
    cdcdata_LineStateUpdated[1] = 0;
    cdcdata_TimeoutCount[1] = 0;
    cdcdata_ZLPPending[1] = 0;
    cdcdata_Lock[1] = 0;

    cdcdata_Outbdp[1] = &usb_bdt[USB_CALC_BD(4, USB_DIR_OUT, USB_PP_EVEN)];
    cdcdata_Inbdp[1] = &usb_bdt[USB_CALC_BD(4, USB_DIR_IN, USB_PP_EVEN)];

    cdcdata_InLen[1] = 0;
    cdcdata_IsInBufferA[1] = 0xFF;
    cdcdata_InPtr[1] = cdcdata1_InBufferA;
    cdcdata_Inbdp[1]->BDADDR = &cdcdata1_InBufferA[0];
    cdcdata_Inbdp[1]->BDCNT = 0;
    cdcdata_Inbdp[1]->BDSTAT = DTS + DTSEN;

    cdcdata_OutLen[1] = 0;
    cdcdata_IsOutBufferA[1] = 0xFF;
    cdcdata_OutPtr[1] = cdcdata1_OutBufferA;
    cdcdata_Outbdp[1]->BDCNT = CDC_BUFFER_SIZE;
    cdcdata_Outbdp[1]->BDADDR = &cdcdata1_OutBufferA[0];
    cdcdata_Outbdp[1]->BDSTAT = UOWN + DTSEN;

    usb_register_class_setup_handler(cdc_setup);
    usb_register_sof_handler(cdc_flush_on_timeout);
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
                    memcpy(EP0_Inbdp->BDADDR, (const void *) &(cdcdata_LineCodeing[0]), reply_len);
                    usb_ack_dat1(reply_len); // JTR common addition for STD and CLASS ACK
                    usb_set_in_handler(0, cdc_get_line_coding);
                    break;

                case CDC_SET_CONTROL_LINE_STATE: // Optional
                    cdcdata_ControlLineState[0] = *((struct _cdc_ControlLineState *) &packet[USB_wValue]);
                    usb_set_in_handler(0, cdc_set_control_line_state_status); // JTR why bother?
                    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK
                    cdcdata_LineStateUpdated[0] = 1;
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

    memcpy(&cdcdata_LineCodeing[0], (const void *) EP0_Outbdp->BDADDR, sizeof (struct cdc_LineCodeing));

    dwBaud = BAUDCLOCK_FREQ / cdcdata_LineCodeing[0].dwDTERate;
    dwBaudrem = BAUDCLOCK_FREQ % cdcdata_LineCodeing[0].dwDTERate;
    if (cdcdata_LineCodeing[0].dwDTERate > (dwBaudrem << 1))
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
void cdc_wait_out_ready(BYTE port) // JTR2 added reduced overhead
{
    while ((cdcdata_Outbdp[port]->BDSTAT & UOWN));
}

/******************************************************************************/

void cdc_wait_in_ready(BYTE port) // JTR2 added reduced overhead
{
    while ((cdcdata_Inbdp[port]->BDSTAT & UOWN));
}//end cdc_wait_in_ready

/******************************************************************************/
BYTE cdc_out_ready(BYTE port) {
    return !(cdcdata_Outbdp[port]->BDSTAT & UOWN); // Do we have a packet from host?
}

/******************************************************************************/
BYTE cdc_in_ready(BYTE port) {
    return !(cdcdata_Inbdp[port]->BDSTAT & UOWN); // Is the CDC In buffer ready?
}

/******************************************************************************/
BYTE cdc_getda(BYTE port) {
    CDCFunctionError = 0;
    cdc_wait_out_ready(port);
    
    if ((cdcdata_IsOutBufferA[port] & 1)) {
        switch(port) {
        case 0:
            cdcdata_OutPtr[0] = &cdcdata0_OutBufferA[0];
            cdcdata_Outbdp[0]->BDADDR = &cdcdata0_OutBufferB[0];
            break;
        case 1:
            cdcdata_OutPtr[1] = &cdcdata1_OutBufferA[0];
            cdcdata_Outbdp[1]->BDADDR = &cdcdata1_OutBufferB[0];
            break;
	}
    } else {
        switch(port) {
        case 0:
            cdcdata_OutPtr[0] = &cdcdata0_OutBufferB[0];
            cdcdata_Outbdp[0]->BDADDR = &cdcdata0_OutBufferA[0];
            break;
        case 1:
            cdcdata_OutPtr[1] = &cdcdata1_OutBufferB[0];
            cdcdata_Outbdp[1]->BDADDR = &cdcdata1_OutBufferA[0];
            break;
        }
    }

    cdcdata_IsOutBufferA[port] ^= 0xFF;
    cdcdata_OutLen[port] = cdcdata_Outbdp[port]->BDCNT;
    cdcdata_Outbdp[port]->BDCNT = CDC_BUFFER_SIZE;
    cdcdata_Outbdp[port]->BDSTAT = ((cdcdata_Outbdp[port]->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
#ifndef USB_INTERRUPTS
    usb_handler();
#endif
    return cdcdata_OutLen[port];
}//end getCDC_Out_ArmNext

BYTE cdc_putda(BYTE port, BYTE count);
BYTE cdc_putda(BYTE port, BYTE count) {
    // When calling this function, cdcdata_Lock[port] == 1;
    if(cdcdata_Lock[port] != 1)
        return 0;

    cdc_wait_in_ready(port);
    if ((cdcdata_IsInBufferA[port] & 1)) {
        switch(port) {
        case 0:
            cdcdata_InPtr[0] = &cdcdata0_InBufferA[0];
            cdcdata_Inbdp[0]->BDADDR = &cdcdata0_InBufferB[0];
            break;
        case 1:
            cdcdata_InPtr[1] = &cdcdata1_InBufferA[0];
            cdcdata_Inbdp[1]->BDADDR = &cdcdata1_InBufferB[0];
            break;
	}
    } else {
        switch(port) {
        case 0:
            cdcdata_InPtr[0] = &cdcdata0_InBufferB[0];
            cdcdata_Inbdp[0]->BDADDR = &cdcdata0_InBufferA[0];
            break;
        case 1:
            cdcdata_InPtr[1] = &cdcdata1_InBufferB[0];
            cdcdata_Inbdp[1]->BDADDR = &cdcdata1_InBufferA[0];
            break;
        }
    }

    cdcdata_Inbdp[port]->BDCNT = count;
    cdcdata_Inbdp[port]->BDSTAT = ((cdcdata_Inbdp[port]->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    cdcdata_IsInBufferA[port] ^= 0xFF;
#ifndef USB_INTERRUPTS
    usb_handler();
#endif
    return 0; //CDCFunctionError;
}

/******************************************************************************/
void cdc_flush_in_now(BYTE port) {
    cdcdata_Lock[port] = 1;
    // {
    if (cdcdata_InLen[port] > 0) {
        cdc_putda(port, cdcdata_InLen[port]);
        if (cdcdata_InLen[port] == CDC_BUFFER_SIZE) {
            cdcdata_ZLPPending[port] = 1;
        } else {
            cdcdata_ZLPPending[port] = 0;
        }
        cdcdata_InLen[port] = 0;
        cdcdata_TimeoutCount[port] = 0;
    } else if (cdcdata_ZLPPending[port]) {
        cdc_putda(port, 0);
        cdcdata_ZLPPending[port] = 0;
        cdcdata_TimeoutCount[port] = 0;
    }
    // }
    cdcdata_Lock[port] = 0;
    return;
}

/******************************************************************************/
void cdc_flush_on_timeout_port(BYTE port) {
    if (cdcdata_TimeoutCount[port] >= CDC_FLUSH_MS) {
        if(!cdcdata_Lock[port]) {
            cdc_flush_in_now(port);
        }
    } else {
        cdcdata_TimeoutCount[port]++;
    }
}

void cdc_flush_on_timeout(void) {
    cdc_flush_on_timeout_port(0);
    cdc_flush_on_timeout_port(1);
}

/******************************************************************************/
void cdc_putc(BYTE port, BYTE c) {
    cdcdata_Lock[port] = 1;
    // {
    *(cdcdata_InPtr[port]) = c;
    cdcdata_InPtr[port]++;
    cdcdata_InLen[port]++;
    cdcdata_ZLPPending[port] = 0;
    if (cdcdata_InLen[port] == CDC_BUFFER_SIZE) {
        cdc_putda(port, cdcdata_InLen[port]);
        cdcdata_InLen[port] = 0;
        cdcdata_ZLPPending[port] = 1;
    }
    cdcdata_TimeoutCount[port] = 0; //setup timer to throw data if the buffer doesn't fill
    // }
    cdcdata_Lock[port] = 0;
    return;
}

void cdc_put_cstr(BYTE port, const rom char* msg) {
    while(*msg != '\0') {
        cdc_putc(port, *msg);
        msg++;
    }
}

/******************************************************************************/
// Waits for a byte to be available and returns that byte as a
// function return value. The byte is removed from the CDC OUT queue.
// No return count is required as this function always returns one byte.
BYTE cdc_getc(BYTE port) { // Must be used only in double buffer mode.
    BYTE c = 0;

    if (cdcdata_OutLen[port] == 0) {
        do {
            cdcdata_OutLen[port] = cdc_getda(port);
        } while (cdcdata_OutLen[port] == 0); // Skip any ZLP
    }
    c = *(cdcdata_OutPtr[port]);
    cdcdata_OutPtr[port]++;
    cdcdata_OutLen[port]--;
    return c;
}

/******************************************************************************/
// Checks to see if there is a byte available in the CDC buffer.
// If so, it returns that byte at the dereferenced pointer *C
// and the function returns a count of 1. The byte is effectively
// removed from the queue.
// IF no byte is available function returns immediately with a count of zero.
/*
BYTE cdc_poll_getc(BYTE port, BYTE * c) { // Must be used only in double buffer mode.
    if (cdcdata_OutLen[port]) { // Do we have a byte waiting?
        *c = *(cdcdata_OutPtr[port]); // pass it on and adjust cdcdata_OutPtr and count
        cdcdata_OutPtr[port]++;
        cdcdata_OutLen[port]--;
        return 1; // Return byte count, always one.
    }
    if (cdc_out_ready(port)) { // No byte in queue check for new arrivals.
        cdcdata_OutLen[port] = cdc_getda(port);
        if (cdcdata_OutLen[port]) {
            *c = *(cdcdata_OutPtr[port]);
            cdcdata_OutPtr[port]++;
            cdcdata_OutLen[port]--;
            return 1;
        }
    }
    return 0;
}
*/

/******************************************************************************/
// Checks (PEEKS) to see if there is a byte available in the CDC buffer.
// If so, it returns that byte at the dereferenced pointer *C
// and the function returns a count of 1. The byte however is NOT
// removed from the queue and can still be read with the cdc_poll_getc()
// and getc_cdc() functions that will remove it from the queue.
// IF no byte is available function returns immediately with a count of zero.
BYTE cdc_peek_getc(BYTE port, BYTE * c) { // Must be used only in double buffer mode.
    if (cdcdata_OutLen[port]) {
        *c = *(cdcdata_OutPtr[port]);
        return 1;
    }
    if (cdc_out_ready(port)) {
        cdcdata_OutLen[port] = cdc_getda(port);
        if (cdcdata_OutLen[port]) {
            *c = *(cdcdata_OutPtr[port]);
            return 1;
        }
    }
    return 0;
}
