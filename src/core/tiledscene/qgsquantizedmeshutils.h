/***************************************************************************
                             qgsquantizedmeshutils.h
                             ----------------------------
    begin                : May 2024
    copyright            : (C) 2024 by David Koňařík
    email                : dvdkon at konarici dot cz

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "qgis_core.h"
#include "qgstiledscenetile.h"

#define TINYGLTF_NO_STB_IMAGE         // we use QImage-based reading of images
#define TINYGLTF_NO_STB_IMAGE_WRITE   // we don't need writing of images
#include "tiny_gltf.h"

// Definition copied from format spec: https://github.com/CesiumGS/quantized-mesh
#pragma pack (push, 1)
struct CORE_EXPORT QgsQuantizedMeshHeader
{
  // The center of the tile in Earth-centered Fixed coordinates.
  double CenterX;
  double CenterY;
  double CenterZ;

  // The minimum and maximum heights in the area covered by this tile.
  // The minimum may be lower and the maximum may be higher than
  // the height of any vertex in this tile in the case that the min/max vertex
  // was removed during mesh simplification, but these are the appropriate
  // values to use for analysis or visualization.
  float MinimumHeight;
  float MaximumHeight;

  // The tile’s bounding sphere.  The X,Y,Z coordinates are again expressed
  // in Earth-centered Fixed coordinates, and the radius is in meters.
  double BoundingSphereCenterX;
  double BoundingSphereCenterY;
  double BoundingSphereCenterZ;
  double BoundingSphereRadius;

  // The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame.
  // If this point is below the horizon, the entire tile is below the horizon.
  // See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
  double HorizonOcclusionPointX;
  double HorizonOcclusionPointY;
  double HorizonOcclusionPointZ;

  uint32_t vertexCount;
};
#pragma pack (pop)

struct CORE_EXPORT QgsQuantizedMeshTile
{
  QgsQuantizedMeshHeader mHeader;
  std::vector<uint16_t> mVertexCoords;
  std::vector<uint32_t> mTriangleIndices;
  std::vector<uint32_t> mWestVertices;
  std::vector<uint32_t> mSouthVertices;
  std::vector<uint32_t> mEastVertices;
  std::vector<uint32_t> mNorthVertices;
  std::map<uint8_t, std::vector<char>> mExtensions;

  QgsQuantizedMeshTile( const std::vector<char> &data );
  tinygltf::Model toGltf();
};
