///////////////////////////////////////////////////////////////////////////////
///
///	\file    GenerateRLLMesh.cpp
///	\author  Paul Ullrich
///	\version March 7, 2014
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

#include "GridElements.h"
#include "DataArray1D.h"
#include "Exception.h"
#include "Announce.h"
#include "STLStringHelper.h"
#include "NetCDFUtilities.h"

#include <cmath>
#include <iostream>

#include "netcdfcpp.h"

///////////////////////////////////////////////////////////////////////////////

#define ONLY_GREAT_CIRCLES


///////////////////////////////////////////////////////////////////////////////
// 
// Input Parameters:
// Number of longitudes in mesh: int nLongitudes;
// Number of latitudes in mesh: int nLatitudes;
// First longitude line on mesh: double dLonBegin;
// Last longitude line on mesh: double dLonEnd;
// First latitude line on mesh: double dLatBegin;
// Last latitude line on mesh: double dLatEnd;
// Flip latitude and longitude dimension in FaceVector ordering: bool fFlipLatLon;
// Output filename:  std::string strOutputFile;
// 
extern "C" 
int GenerateRLLMesh(
	Mesh & mesh, 
	int nLongitudes,
	int nLatitudes, 
	double dLonBegin,
	double dLonEnd, 
	double dLatBegin,
	double dLatEnd, 
	bool fGlobalCap,
	bool fFlipLatLon,
	bool fForceGlobal,
	std::string strInputFile,
	std::string strInputFileLonName,
	std::string strInputFileLatName,
	std::string strOutputFile, 
	std::string strOutputFormat,
	bool fVerbose
) {

	NcError error(NcError::silent_nonfatal);

try {

    // Check command line parameters (data type arguments)
    STLStringHelper::ToLower(strOutputFormat);

	NcFile::FileFormat eOutputFormat =
		GetNcFileFormatFromString(strOutputFormat);
	if (eOutputFormat == NcFile::BadFormat) {
		_EXCEPTION1("Invalid \"out_format\" value (%s), "
			"expected [Classic|Offset64Bits|Netcdf4|Netcdf4Classic]",
			strOutputFormat.c_str());
	}

	// Check fGlobalCap argument
	bool fCapBegin = false;
	bool fCapEnd = false;
	if (fGlobalCap) {
		if (strInputFile != "") {
			_EXCEPTIONT("--global_cap cannot be applied with --in_file");
		}
		if (fabs(dLonEnd - dLonBegin) - 360.0 >= 1.0e-12) {
			_EXCEPTIONT("--global_cap only allowed for 360 degrees of longitude");
		}
		if (fabs(fabs(dLatBegin) - 90.0) < 1.0e-12) {
			fCapBegin = true;
			std::cout << "Applying global_cap to lat_begin" << std::endl;
		}
		if (fabs(fabs(dLatEnd) - 90.0) < 1.0e-12) {
			fCapEnd = true;
			std::cout << "Applying global_cap to lat_end" << std::endl;
		}
		if (!fCapBegin && !fCapEnd) {
			std::cout << "WARNING: global_cap only applied if pole points are included in latitude"
				" range -- ignoring" << std::endl;
		}
	}

	// Longitude and latitude arrays
	DataArray1D<double> dLonEdge;
	DataArray1D<double> dLatEdge;

	// Generate mesh from input datafile
	if (strInputFile != "") {

		std::cout << "Generating mesh from input datafile ";
		std::cout << "\"" << strInputFile << "\"" << std::endl;

		NcFile ncfileInput(strInputFile.c_str(), NcFile::ReadOnly);
		if (!ncfileInput.is_valid()) {
			_EXCEPTION1("Unable to load input file \"%s\"", strInputFile.c_str());
		}

		NcDim * dimLon = ncfileInput.get_dim(strInputFileLonName.c_str());
		if (dimLon == NULL) {
			_EXCEPTION1("Input file missing dimension \"%s\"",
				strInputFileLonName.c_str());
		}

		NcDim * dimLat = ncfileInput.get_dim(strInputFileLatName.c_str());
		if (dimLat == NULL) {
			_EXCEPTION1("Input file missing dimension \"%s\"",
				strInputFileLatName.c_str());
		}

		NcVar * varLon = ncfileInput.get_var(strInputFileLonName.c_str());
		if (varLon == NULL) {
			_EXCEPTION1("Input file missing variable \"%s\"",
				strInputFileLonName.c_str());
		}

		NcVar * varLat = ncfileInput.get_var(strInputFileLatName.c_str());
		if (varLon == NULL) {
			_EXCEPTION1("Input file missing variable \"%s\"",
				strInputFileLatName.c_str());
		}

		nLongitudes = dimLon->size();
		nLatitudes = dimLat->size();

		if (nLongitudes < 2) {
			_EXCEPTIONT("At least two longitudes required in input file");
		}
		if (nLatitudes < 2) {
			_EXCEPTIONT("At least two latitudes required in input file");
		}

		DataArray1D<double> dLonNode(nLongitudes);
		varLon->set_cur((long)0);
		varLon->get(&(dLonNode[0]), nLongitudes);

		DataArray1D<double> dLatNode(nLatitudes);
		varLat->set_cur((long)0);
		varLat->get(&(dLatNode[0]), nLatitudes);

		double dLatOrient = 1.0;
		if (dLatNode[1] < dLatNode[0]) {
			dLatOrient = -1.0;
		}
		for (int i = 0; i < nLatitudes-1; i++) {
			if (dLatOrient * dLatNode[i] > dLatOrient * dLatNode[i+1]) {
				_EXCEPTIONT("Latitudes must be monotone increasing or decreasing");
			}
		}

		for (int i = 0; i < nLongitudes-1; i++) {
			if (dLonNode[i] > dLonNode[i+1]) {
				_EXCEPTIONT("Longitudes must be monotone increasing");
			}
		}
/*
		for (int j = 0; j < nLatitudes-1; j++) {
			if (dLatNode[j] > dLatNode[j+1]) {
				_EXCEPTIONT("Latitudes must be monotone increasing");
			}
		}
*/
		double dFirstDeltaLon = dLonNode[1] - dLonNode[0];
		double dSecondLastDeltaLon = dLonNode[nLongitudes-1] - dLonNode[nLongitudes-2];
		double dLastDeltaLon = dLonNode[0] - (dLonNode[nLongitudes-1] - 360.0);

		if (fabs(dFirstDeltaLon - dLastDeltaLon) < 1.0e-12) {
			std::cout << "..Mesh assumed periodic in longitude" << std::endl;
			fForceGlobal = true;
		}

		// Initialize longitude edges
		dLonEdge.Allocate(nLongitudes+1);

		if (fForceGlobal) {
			dLonEdge[0] = 0.5 * (dLonNode[0] + dLonNode[nLongitudes-1] - 360.0);
			dLonEdge[nLongitudes] = dLonEdge[0] + 360.0;
		} else {
			dLonEdge[0] = dLonNode[0] - 0.5 * dFirstDeltaLon;
			dLonEdge[nLongitudes] = dLonNode[nLongitudes-1] + 0.5 * dSecondLastDeltaLon;
		}

		for (int i = 1; i < nLongitudes; i++) {
			dLonEdge[i] = 0.5 * (dLonNode[i-1] + dLonNode[i]);
		}

		// Initialize latitude edges
		dLatEdge.Allocate(nLatitudes+1);

		dLatEdge[0] =
			dLatNode[0]
			- 0.5 * (dLatNode[1] - dLatNode[0]);

		if (dLatEdge[0] < -90.0) {
			dLatEdge[0] = -90.0;
		}
		if (dLatEdge[0] > 90.0) {
			dLatEdge[0] = 90.0;
		}

		dLatEdge[nLatitudes] =
			dLatNode[nLatitudes-1]
			+ 0.5 * (dLatNode[nLatitudes-1] - dLatNode[nLatitudes-2]);

		if (dLatEdge[nLatitudes] > 90.0) {
			dLatEdge[nLatitudes] = 90.0;
		}
		if (dLatEdge[nLatitudes] < -90.0) {
			dLatEdge[nLatitudes] = -90.0;
		}

		for (int j = 1; j < nLatitudes; j++) {
			dLatEdge[j] = 0.5 * (dLatNode[j-1] + dLatNode[j]);
		}

		// Convert all longitudes and latitudes to radians
		for (int i = 0; i <= nLongitudes; i++) {
			dLonEdge[i] *= M_PI / 180.0;
		}
		for (int j = 0; j <= nLatitudes; j++) {
			dLatEdge[j] *= M_PI / 180.0;
		}

		// Convert latitude and longitude interval to radians
		dLonBegin = dLonEdge[0];
		dLonEnd = dLonEdge[nLongitudes];
		dLatBegin = dLatEdge[0];
		dLatEnd = dLatEdge[nLatitudes];

	// Generate mesh from parameters
	} else {

		// Convert latitude and longitude interval to radians
		dLonBegin *= M_PI / 180.0;
		dLonEnd   *= M_PI / 180.0;
		dLatBegin *= M_PI / 180.0;
		dLatEnd   *= M_PI / 180.0;

		// Create longitude arrays
		double dDeltaLon = dLonEnd - dLonBegin;
		double dDeltaLat = dLatEnd - dLatBegin;

		dLonEdge.Allocate(nLongitudes+1);
		for (int i = 0; i <= nLongitudes; i++) {
			double dLambdaFrac =
				  static_cast<double>(i)
				/ static_cast<double>(nLongitudes);

			dLonEdge[i] = dDeltaLon * dLambdaFrac + dLonBegin;
		}

		// Allocate latitude arrays
		dLatEdge.Allocate(nLatitudes+1);

		// Create latitude arrays (no caps)
		if (!fCapBegin && !fCapEnd) {
			for (int j = 0; j <= nLatitudes; j++) {
				double dPhiFrac =
					  static_cast<double>(j)
					/ static_cast<double>(nLatitudes);

				dLatEdge[j] = dDeltaLat * dPhiFrac + dLatBegin;
			}

		// Create latitude arrays (cap both)
		} else if (fCapBegin && fCapEnd) {
			dLatEdge[0] = dLatBegin;
			for (int j = 1; j < nLatitudes; j++) {
				double dPhiFrac =
					  (static_cast<double>(j) - 0.5)
					/ static_cast<double>(nLatitudes - 1);

				dLatEdge[j] = dDeltaLat * dPhiFrac + dLatBegin;
			}
			dLatEdge[nLatitudes] = dLatEnd;

		// Create latitude arrays (cap begin)
		} else if (fCapBegin) {
			dLatEdge[0] = dLatBegin;
			for (int j = 1; j <= nLatitudes; j++) {
				double dPhiFrac =
					  (static_cast<double>(j) - 0.5)
					/ (static_cast<double>(nLatitudes) - 0.5);

				dLatEdge[j] = dDeltaLat * dPhiFrac + dLatBegin;
			}

		// Create latitude arrays (cap end)
		} else if (fCapEnd) {
			for (int j = 0; j < nLatitudes; j++) {
				double dPhiFrac =
					  static_cast<double>(j)
					/ (static_cast<double>(nLatitudes) - 0.5);

				dLatEdge[j] = dDeltaLat * dPhiFrac + dLatBegin;
			}
			dLatEdge[nLatitudes] = dLatEnd;
		}
	}

	// Output longitude / latitude arrays
	if (fVerbose) {
		std::cout << "longitude_edges = [";
		for (int i = 0; i <= nLongitudes; i++) {
			std::cout << dLonEdge[i] * 180.0 / M_PI;
			if (i != nLongitudes) {
				std::cout << ", ";
			}
		}
		std::cout << "]" << std::endl;
		std::cout << "latitude_edges = [";
		for (int j = 0; j <= nLatitudes; j++) {
			std::cout << dLatEdge[j] * 180.0 / M_PI;
			if (j != nLatitudes) {
				std::cout << ", ";
			}
		}
		std::cout << "]" << std::endl;
	}

	std::cout << "..Generating mesh with resolution [";
	std::cout << nLongitudes << ", " << nLatitudes << "]" << std::endl;
	std::cout << "..Longitudes in range [";
	std::cout << dLonBegin * 180.0 / M_PI << ", " << dLonEnd * 180.0 / M_PI << "]" << std::endl;
	std::cout << "..Latitudes in range [";
	std::cout << dLatBegin * 180.0 / M_PI << ", " << dLatEnd * 180.0 / M_PI << "]" << std::endl;

	// Check parameters
	if (nLatitudes < 2) {
		std::cout << "Error: At least 2 latitudes are required." << std::endl;
		return -5; // Argument error
	}
	if (nLongitudes < 2) {
		std::cout << "Error: At least 2 longitudes are required." << std::endl;
		return -5; // Argument error
	}

	NodeVector & nodes = mesh.nodes;
	FaceVector & faces = mesh.faces;
    mesh.type = Mesh::MeshType_RLL;

	// Check if longitudes wrap
	bool fWrapLongitudes = false;
	if (fmod(dLonEnd - dLonBegin, 2.0 * M_PI) < 1.0e-12) {
		fWrapLongitudes = true;
	}
	bool fIncludeSouthPole = (fabs(dLatBegin + 0.5 * M_PI) < 1.0e-12);
	bool fIncludeNorthPole = (fabs(dLatEnd   - 0.5 * M_PI) < 1.0e-12);

	int iSouthPoleOffset = (fIncludeSouthPole)?(1):(0);

	// Increase number of latitudes if south pole is not included
	int iInteriorLatBegin = (fIncludeSouthPole)?(1):(0);
	int iInteriorLatEnd   = (fIncludeNorthPole)?(nLatitudes-1):(nLatitudes);

	// Number of longitude nodes
	int nLongitudeNodes = nLongitudes;
	if (!fWrapLongitudes) {
		nLongitudeNodes++;
	}

	// Generate nodes
	if (fIncludeSouthPole) {
		nodes.push_back(Node(0.0, 0.0, -1.0));
	}
	for (int j = iInteriorLatBegin; j < iInteriorLatEnd+1; j++) {
		for (int i = 0; i < nLongitudeNodes; i++) {

			double dLambda = dLonEdge[i];
			double dPhi = dLatEdge[j];

			double dX = cos(dPhi) * cos(dLambda);
			double dY = cos(dPhi) * sin(dLambda);
			double dZ = sin(dPhi);

			nodes.push_back(Node(dX, dY, dZ));
		}
	}
	if (fIncludeNorthPole) {
		nodes.push_back(Node(0.0, 0.0, +1.0));
	}

	// Flip orientation
	bool fFlipOrientation = false;

	if (dLatEdge[1] < dLatEdge[0]) {
		fFlipOrientation = !fFlipOrientation;
	}
	if (dLonEdge[1] < dLonEdge[0]) {
		fFlipOrientation = !fFlipOrientation;
	}

	// Generate south polar faces
	if (fIncludeSouthPole) {
		for (int i = 0; i < nLongitudes; i++) {
			Face face(4);
			face.SetNode(0, 0);
			face.SetNode(2, i + 1);
			if (!fFlipOrientation) {
				face.SetNode(1, (i+1) % nLongitudeNodes + 1);
				face.SetNode(3, 0);
			} else {
				face.SetNode(1, 0);
				face.SetNode(3, (i+1) % nLongitudeNodes + 1);
			}

#ifndef ONLY_GREAT_CIRCLES
			face.edges[1].type = Edge::Type_ConstantLatitude;
			face.edges[3].type = Edge::Type_ConstantLatitude;
#endif

			faces.push_back(face);
		}
	}

	// Generate interior faces
	for (int j = iInteriorLatBegin; j < iInteriorLatEnd; j++) {
		int jx = j - iInteriorLatBegin;

		int iThisLatNodeIx =  jx    * nLongitudeNodes + iSouthPoleOffset;
		int iNextLatNodeIx = (jx+1) * nLongitudeNodes + iSouthPoleOffset;

		for (int i = 0; i < nLongitudes; i++) {
			Face face(4);
			face.SetNode(0, iThisLatNodeIx + (i + 1) % nLongitudeNodes);
			face.SetNode(2, iNextLatNodeIx + i);

			if (!fFlipOrientation) {
				face.SetNode(1, iNextLatNodeIx + (i + 1) % nLongitudeNodes);
				face.SetNode(3, iThisLatNodeIx + i);
			} else {
				face.SetNode(1, iThisLatNodeIx + i);
				face.SetNode(3, iNextLatNodeIx + (i + 1) % nLongitudeNodes);
			}
#ifndef ONLY_GREAT_CIRCLES
			face.edges[1].type = Edge::Type_ConstantLatitude;
			face.edges[3].type = Edge::Type_ConstantLatitude;
#endif

			faces.push_back(face);
		}
	}

	// Generate north polar faces
	if (fIncludeNorthPole) {
		int jx = nLatitudes - iInteriorLatBegin - 1;

		int iThisLatNodeIx =  jx    * nLongitudeNodes + iSouthPoleOffset;
		int iNextLatNodeIx = (jx+1) * nLongitudeNodes + iSouthPoleOffset;

		int iNorthPolarNodeIx = static_cast<int>(nodes.size()-1);
		for (int i = 0; i < nLongitudes; i++) {
			Face face(4);
			face.SetNode(0, iNorthPolarNodeIx);
			face.SetNode(2, iThisLatNodeIx + (i + 1) % nLongitudeNodes);

			if (!fFlipOrientation) {
				face.SetNode(1, iThisLatNodeIx + i);
				face.SetNode(3, iNorthPolarNodeIx);
			} else {
				face.SetNode(1, iNorthPolarNodeIx);
				face.SetNode(3, iThisLatNodeIx + i);
			}

#ifndef ONLY_GREAT_CIRCLES
			face.edges[1].type = Edge::Type_ConstantLatitude;
			face.edges[3].type = Edge::Type_ConstantLatitude;
#endif

			faces.push_back(face);
		}
	}

	// Reorder the faces
	if (fFlipLatLon) {
		FaceVector faceTemp = mesh.faces;
		mesh.faces.clear();
		for (int i = 0; i < nLongitudes; i++) {
		for (int j = 0; j < nLatitudes; j++) {
			mesh.faces.push_back(faceTemp[j * nLongitudes + i]);
		}
		}
	}

	// Output the mesh
	if (strOutputFile.size()) {

		// Announce
		std::cout << "..Writing mesh to file [" << strOutputFile.c_str() << "] ";
		std::cout << std::endl;

		mesh.Write(strOutputFile, eOutputFormat);

		// Add rectilinear properties
		//if (!fIncludeSouthPole) {
		//	nLatitudes--;
		//}

		NcFile ncOutput(strOutputFile.c_str(), NcFile::Write);
		ncOutput.add_att("rectilinear", "true");

		if (fFlipLatLon) {
			ncOutput.add_att("rectilinear_dim0_size", nLongitudes);
			ncOutput.add_att("rectilinear_dim1_size", nLatitudes);
			ncOutput.add_att("rectilinear_dim0_name", "lon");
			ncOutput.add_att("rectilinear_dim1_name", "lat");
		} else {
			ncOutput.add_att("rectilinear_dim0_size", nLatitudes);
			ncOutput.add_att("rectilinear_dim1_size", nLongitudes);
			ncOutput.add_att("rectilinear_dim0_name", "lat");
			ncOutput.add_att("rectilinear_dim1_name", "lon");
		}
		ncOutput.close();
	}

	// Announce
	std::cout << "..Mesh generator exited successfully" << std::endl;
	std::cout << "=========================================================";
	std::cout << std::endl;

  return 0;

} catch(Exception & e) {
	Announce(e.ToString().c_str());
	return (0);

} catch(...) {
	return (0);
}
}

///////////////////////////////////////////////////////////////////////////////
