#pragma once

#include "LsITetrahedronProcessor.h"
#include "LsLatticeVertexRef.h"
#include <unordered_map>

class LsSpringMeshBuilder: public LsITetrahedronProcessor {
private:
	typedef struct {
 		double edgeAdjacentTetrahedronVolume;
	} LsTmpSpringInfo;
	typedef std::unordered_map<LsLatticeVertexRef, LsTmpSpringInfo> LsTmpSpringMeshNodeInfo;
	std::unordered_map<LsLatticeVertexRef, LsTmpSpringMeshNodeInfo> temporarySpringMesh;
public:

    // @ void LsSpringMeshBuilder::Reset()
	//-------------------------------------------------------------------------------
    // Reset intermediate buffers and start new mesh.
    //-------------------------------------------------------------------------------
    void Reset();

	// @ void LsSpringMeshBuilder::OnTetrahedronEmitted(...)
	//-------------------------------------------------------------------------------
    // LsSpringMeshBuilder class will receive tetrahedra as they are being filled 
    // through this callback and store in temporary data structure.
    //-------------------------------------------------------------------------------
    void OnTetrahedronEmitted(const LsLatticeVertexRef& ref1, const LsLatticeVertexRef& ref2, const LsLatticeVertexRef& ref3, const LsLatticeVertexRef& ref4);

    // @ LsSpringMesh LsSpringMeshBuilder::Build()
	//-------------------------------------------------------------------------------
    // Process intermediate buffers and produce a spring mesh.
    //-------------------------------------------------------------------------------
    void Build();
};

