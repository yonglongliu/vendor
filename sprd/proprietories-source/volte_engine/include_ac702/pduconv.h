#ifndef _PDU_H_
#define _PDU_H_

int ascii_to_pdu(
    char *ascii,
    unsigned char *pdu,
    int maxTargetLen,
    int *hasUnknownChars);

int pdu_to_ascii(
    unsigned char *pdu,
    int pdulength,
    char *ascii,
    int maxTargetLen);

int count_septets(
    char *ascii);

int pdu_utf16_to_utf8(
    unsigned short *in_ptr,
    int inLen,
    unsigned char  *out_ptr,
    int outLen);

int ascii_to_7bitAsciiPdu(
    char *ascii,
    unsigned char *pdu,
    int maxTargetLen,
    int *hasUnknownChars);

int pdu7BitAscii_to_ascii(
    unsigned char *pdu,
    int            pdulength,
    char          *ascii,
    int            maxTargetLen);

int ascii_to_7bitAsciiAnsi(
    char *ascii,
    unsigned char *pdu,
    int maxTargetLen,
    int *hasUnknownChars);

int ansi7bitAscii_to_ascii(
    unsigned char *pdu,
    int            pdulength,
    char          *ascii,
    int            maxTargetLen);

#endif
