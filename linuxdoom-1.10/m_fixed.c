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
#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"

#ifdef __GNUG__
#pragma implementation "m_fixed.h"
#endif
#include "m_fixed.h"




// Fixme. __USE_C_FIXED__ or something.

fixed_t
FixedMul
( fixed_t	a,
  fixed_t	b )
{
#ifdef __M68000__
    uint16_t alw = a;
    int16_t ahw = a >> FRACBITS;
    uint16_t blw = b;
    int16_t bhw = b >> FRACBITS;

    if (bhw == 0) {
        uint32_t ll = (uint32_t) alw * blw;
        int32_t hl = ( int32_t) ahw * blw;
        return (ll >> FRACBITS) + hl;
    } else if (alw == 0) {
        //return ahw * b;
        int32_t hl = ( int32_t) ahw * blw;
        int32_t hh = ( int32_t) ahw * bhw;
        return hl + (hh << FRACBITS);
    } else {
        uint32_t ll = (uint32_t) alw * blw;
        int32_t hl = ( int32_t) ahw * blw;
        return (a * bhw) + (ll >> FRACBITS) + hl;
    }
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
    uint16_t alw = a;
    int16_t ahw = a >> FRACBITS;
    uint16_t blw = b < 0 ? -2*b : 2*b;

    uint32_t ll = (uint32_t) alw * blw;
    int32_t hl = ( int32_t) ahw * blw;
    fixed_t result = (ll >> FRACBITS) + hl;
    if (b < 0) result = -result;
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
