// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Fixed point implementation.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";

#include <stdint.h>
#include <stdio.h>
#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"

#ifdef __GNUG__
#pragma implementation "m_fixed.h"
#endif
#include "m_fixed.h"


#ifdef __M68000__

// Emits a mulu.w instruction. It's quite difficult to get gcc to do that :-)
static uint32_t mulu(uint16_t a, uint16_t b) {
#ifdef USEASM
    register uint32_t result = a;
    asm (
        "mulu.w     %[b],%[result]  \n\t"
        : [result] "+d" (result)
        : [b] "d" (b)
        : "cc"
    );
    return result;
#else
    return (uint32_t)a * b;
#endif
}

#endif // __M68000__

// Fixme. __USE_C_FIXED__ or something.

fixed_t
FixedMul
( fixed_t	a,
  fixed_t	b )
{
#ifdef __M68000__
    // Is the result a negative number?
    boolean neg = 0 != ((a ^ b) & 0x80000000);

    // Only work with unsigned numbers.
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    uint16_t alw = a;
    uint16_t ahw = a >> FRACBITS;
    uint16_t blw = b;
    uint16_t bhw = b >> FRACBITS;

    uint32_t hh = ahw && bhw ? mulu(ahw, bhw) << FRACBITS : 0;
    uint32_t hl = ahw && blw ? mulu(ahw, blw) : 0;
    uint32_t lh = alw && bhw ? mulu(alw, bhw) : 0;
    
    // Make sure we round towards -inf
    uint32_t ll = alw && blw ? (mulu(alw, blw) + (neg ? 0xffff : 0)) >> FRACBITS : 0;

    int32_t result = hh + hl + lh + ll;
    if (neg) result = -result;
    return result;
#else
    return ((long long) a * (long long) b) >> FRACBITS;
#endif
}

fixed_t
FixedScale
( fixed_t	a,
  short	b )
{
#ifdef __M68000__
    // Is the result a negative number?
    boolean neg = 0 != ((a ^ b) & 0x80000000);
    if (a < 0) a = -a;
    if (b < 0) b = -b;

    uint16_t alw = a;
    uint16_t ahw = a >> FRACBITS;
    uint16_t blw = 2*b;

    uint32_t hl = mulu(ahw, blw);
    uint32_t ll = mulu(alw, blw) + neg ? 0xffff : 0;
    fixed_t result = hl + (ll >> FRACBITS);
    if (neg) result = -result;
    return result;
#else
    return FixedMul(a,b << 1);
#endif
}


//
// FixedDiv, C version.
//

fixed_t
FixedDiv
( fixed_t	a,
  fixed_t	b )
{
    if ( (abs(a)>>14) >= abs(b))
        return (a^b)<0 ? MININT : MAXINT;
#ifdef __M68000__
    if (a < 0)
        return b<0?FixedDiv2(-a, -b):-FixedDiv2(-a, b);
    else
        return b<0?-FixedDiv2(a, -b):FixedDiv2(a, b);
#else
    return FixedDiv2 (a,b);
#endif
}



fixed_t
FixedDiv2
( fixed_t	a,
  fixed_t	b )
{
#ifdef __M68000__
    uint16_t ibit = 1;
    while (b < a)
    {
        b    <<= 1;
        ibit <<= 1;
    }

    int16_t ch = 0;
    for (; ibit != 0; ibit >>= 1)
    {
        if (a >= b)
        {
            a  -= b;
            ch |= ibit;
        }
        a <<= 1;
    }

    uint16_t cl = 0;
    if (a >= b) {a -= b; cl |= 0x8000;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x4000;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x2000;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x1000;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0800;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0400;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0200;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0100;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0080;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0040;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0020;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0010;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0008;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0004;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0002;} a <<= 1;
    if (a >= b) {a -= b; cl |= 0x0001;}

    return (((fixed_t)ch) << FRACBITS) | cl;
#else
    double c;

    c = ((double)a) / ((double)b) * FRACUNIT;

    if (c >= 2147483648.0 || c < -2147483648.0)
        I_Error("FixedDiv: divide by zero");
    return (fixed_t) c;
#endif
}
