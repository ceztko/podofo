/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef PDF_COMPILER_COMPAT_H
#define PDF_COMPILER_COMPAT_H

//
// *** THIS HEADER IS INCLUDED BY PdfDefines.h ***
// *** DO NOT INCLUDE DIRECTLY ***
#ifndef PDF_DEFINES_H
#error Please include PdfDefines.h instead
#endif

#include "podofo_config.h"

#ifndef PODOFO_COMPILE_RC

// Silence some annoying warnings from Visual Studio
#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#endif // _MSC_VER

// Make sure that DEBUG is defined 
// for debug builds on Windows
// as Visual Studio defines only _DEBUG
#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG 1
#endif // DEBUG
#endif // _DEBUG

#include <cstdint>
#include <cstddef>

#include <podofo/compat/EnumFlags.h>

// Declare ssize_t as a signed alternative to size_t,
// useful for example to provide optional size argument
#if defined(_MSC_VER)
 // Fix missing posix "ssize_t" typedef in MSVC
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
// Posix has ssize_t
#include <sys/types.h>
#endif

// Different compilers express __FUNC__ in different ways and with different
// capabilities. Try to find the best option.
//
// Note that __LINE__ and __FILE__ are *NOT* included.
// Further note that you can't use compile-time string concatenation on __FUNC__ and friends
// on many compilers as they're defined to behave as if they were a:
//    static const char* __func__ = 'nameoffunction';
// just after the opening brace of each function.
//
#if defined(__GNUC__)
#  define PODOFO__FUNCTION__ __PRETTY_FUNCTION__
#else
#  define PODOFO__FUNCTION__ __FUNCTION__
#endif

/**
 * \page PoDoFo PdfCompilerCompat Header
 * 
 * <b>PdfCompilerCompat.h</b> gathers up nastyness required for various
 * compiler compatibility into a central place. All compiler-specific defines,
 * wrappers, and the like should be included here and (if necessary) in
 * PdfCompilerCompat.cpp if they must be visible to public users of the library.
 *
 * If the nasty platform and compiler specific hacks can be kept to PoDoFo's
 * build and need not be visible to users of the library, put them in
 * PdfCompilerCompatPrivate.{cpp,h} instead.
 *
 * Please NEVER use symbols from this header or the PoDoFo::compat namespace in
 * a "using" directive. Always explicitly reference names so it's clear that
 * you're pulling them from the compat cruft.
 */

#endif // !PODOFO_COMPILE_RC

#endif // PDF_COMPILER_COMPAT_H
