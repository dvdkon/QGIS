/***************************************************************************
                             testqgsquantizedmeshutils.cpp
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

// qgis_core doesn't export its tinygltf, so we build our own for the test
#define TINYGLTF_IMPLEMENTATION

#include "qgstest.h"
#include "qgsquantizedmeshutils.h"
#include <fstream>
#include <iostream>

class TestQgsQuantizedMeshUtils : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void tileToGltf();
};

void TestQgsQuantizedMeshUtils::initTestCase()
{
  QgsApplication::init();
  QgsApplication::initQgis();
}

void TestQgsQuantizedMeshUtils::cleanupTestCase()
{
  QgsApplication::exitQgis();
}

void TestQgsQuantizedMeshUtils::tileToGltf()
{
  std::ifstream sampleFile( "sample.terrain" );
  QVERIFY( !sampleFile.fail() );
  sampleFile.seekg( 0, std::ios_base::end );
  size_t sampleLength = sampleFile.tellg();
  sampleFile.seekg( 0, std::ios_base::beg );
  std::vector<char> sampleData( sampleLength );
  sampleFile.read( sampleData.data(), sampleLength );

  auto tile = QgsQuantizedMeshTile( sampleData );

  auto model = tile.toGltf();
  tinygltf::TinyGLTF gltfLoader;
  std::ofstream outFile( "sample.terrain.gltf" );
  gltfLoader.WriteGltfSceneToStream( &model, outFile, true, false );
}


QGSTEST_MAIN( TestQgsQuantizedMeshUtils )
#include "testqgsquantizedmeshutils.moc"
