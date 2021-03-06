///////////////////////////////////////////////////////////////////////////////
///
///	\file    NetCDFUtilities.h
///	\author  Paul Ullrich
///	\version August 14, 2014
///
///	<remarks>
///		Copyright 2000-2014 Paul Ullrich
///
///		This file is distributed as part of the Tempest source code package.
///		Permission is granted to use, copy, modify and distribute this
///		source code and its documentation under the terms of the GNU General
///		Public License.  This software is provided "as is" without express
///		or implied warranty.
///	</remarks>

#ifndef _NETCDFUTILITIES_H_
#define _NETCDFUTILITIES_H_

class NcFile;
class NcVar;

#include "netcdfcpp.h"

#include <string>

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Get the NetCDF file format from a its name.
///	</summary>
NcFile::FileFormat GetNcFileFormatFromString(
	const std::string & strFormat
);

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Copy NetCDF attribute metadata from one file to another.
///	</summary>
void CopyNcFileAttributes(
	NcFile * fileIn,
	NcFile * fileOut
);

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Copy NetCDF attribute metadata from one variable to another.
///	</summary>
void CopyNcVarAttributes(
	NcVar * varIn,
	NcVar * varOut
);

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Copy a NetCDF varaible from one file to another.
///	</summary>
void CopyNcVar(
	NcFile & ncIn,
	NcFile & ncOut,
	const std::string & strVarName,
	bool fCopyAttributes = true
);

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Copy a NetCDF varaible from one file to another if it exists.
///	</summary>
void CopyNcVarIfExists(
	NcFile & ncIn,
	NcFile & ncOut,
	const std::string & strVarName,
	bool fCopyAttributes = true
);

////////////////////////////////////////////////////////////////////////////////

#endif

