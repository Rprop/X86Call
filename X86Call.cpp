/**
 *
 * X86Call Library 1.0.0.0
 *
 * Copyright (c) 2016 rrrfff
 * https://github.com/rrrfff/X86Call
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "X86Call.h"
#include <vadefs.h>
#include <stdarg.h>
#include <basetsd.h>

//-------------------------------------------------------------------------

extern "C" DWORD32 _x86call();
extern "C" DWORD64 _x86push(DWORD32 offset, DWORD32 val);

//-------------------------------------------------------------------------

DWORD32 __cdecl X86Call(DWORD32 func, int argc, ...)
{
	const DWORD32 rsp_reserved = 32; // avail stack start [esp - rsp_reserved]
	_x86push(rsp_reserved + 4, 4 + sizeof(DWORD32) + argc * sizeof(DWORD32)); // arguments count
	_x86push(rsp_reserved + 4 + (argc + 1) * sizeof(DWORD32), func); // function
	va_list args;
	va_start(args, argc);
	while (--argc >= 0) {
		_x86push(rsp_reserved + 8 + argc * sizeof(DWORD32), va_arg(args, DWORD32));;
	}
	va_end(args);

	return _x86call();
}
