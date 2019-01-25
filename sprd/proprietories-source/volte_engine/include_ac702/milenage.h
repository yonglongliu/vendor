/*
 * This file was sourced from 3GPP TS 35.206. The 3GPP provides these
 * algorithms with no associated license.  Please refer to TS 35.205 and
 * 35.206 for details. Below is a link and quote from the 3GPP regarding
 * the milenage algorithms.
 *
 * From http://www.3gpp.org/Confidentiality-Algorithms...
 * "The 3GPP authentication and key generation functions (MILENAGE) have
 * been developed through the collaborative efforts of the 3GPP Organizational
 * Partners.
 *
 * They may be used only for the development and operation of 3G Mobile
 * Communications and services. There are no additional requirements or
 * authorizations necessary for these algorithms to be implemented."
 *
 * $D2Tech$ $Rev: 7735 $ $Date: 2008-09-30 10:59:43 -0500 (Tue, 30 Sep 2008) $
 */

/*-------------------------------------------------------------------
 *          Example algorithms f1, f1*, f2, f3, f4, f5, f5*
 *-------------------------------------------------------------------
 *
 *  A sample implementation of the example 3GPP authentication and
 *  key agreement functions f1, f1*, f2, f3, f4, f5 and f5*.  This is
 *  a byte-oriented implementation of the functions, and of the block
 *  cipher kernel function Rijndael.
 *
 *  This has been coded for clarity, not necessarily for efficiency.
 *
 *  The functions f2, f3, f4 and f5 share the same inputs and have
 *  been coded together as a single function.  f1, f1* and f5* are
 *  all coded separately.
 *
 *-----------------------------------------------------------------*/

#ifndef MILENAGE_H
#define MILENAGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8;


void f1    ( u8 k[16], u8 rand[16], u8 sqn[6], u8 amf[2],
             u8 mac_a[8], u8 op[16] );
void f2345 ( u8 k[16], u8 rand[16],
             u8 res[8], u8 ck[16], u8 ik[16], u8 ak[6], u8 op[16] );
void f1_opc    ( u8 k[16], u8 rand[16], u8 sqn[6], u8 amf[2],
                 u8 mac_a[8], u8 opc[16] );
void f2345_opc ( u8 k[16], u8 rand[16],
                 u8 res[8], u8 ck[16], u8 ik[16], u8 ak[6], u8 opc[16] );
void f1star( u8 k[16], u8 rand[16], u8 sqn[6], u8 amf[2],
             u8 mac_s[8], u8 op[16] );
void f5star( u8 k[16], u8 rand[16],
             u8 ak[6], u8 op[16] );
void ComputeOPc( u8 op_c[16], u8 op[16] );

#ifdef __cplusplus
}
#endif

#endif
