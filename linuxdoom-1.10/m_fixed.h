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
// DESCRIPTION:
//	Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------


#ifndef __M_FIXED__
#define __M_FIXED__


#ifdef __GNUG__
#pragma interface
#endif


//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS		16
#define FRACUNIT		(1<<FRACBITS)

typedef int fixed_t;

// Multiplication of two Q15.16 fixed point values
fixed_t FixedMul	(fixed_t a, fixed_t b);

// Multiplication of a Q15.16 fixed point value with a Q0.15 bit value representing range [-1; 1)
fixed_t FixedScale	(fixed_t a, short b);

// Multiplication of two Q15.16 fixed point values a and b where b is known to be in range (-1; 1).
fixed_t FixedScale32	(fixed_t a, fixed_t b);

// Multiplication of a Q15.16 fixed point value with a 16 bit signed integer
fixed_t FixedMulShort	(fixed_t a, short b);

// Division of two Q15.16 fixed point values.
fixed_t FixedDiv	(fixed_t a, fixed_t b);



#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
