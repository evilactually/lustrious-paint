#pragma once

#include "LsBCCLatticeRef.h"
#include "LsITetrahedronProcessor.h"
#include "LsHashedBCCLatticeVertex.h"
#include <unordered_map>

class LsBCCLattice;

class LsSpringMeshBuilder: public LsITetrahedronProcessor {
private:
	typedef struct {
 		double edgeAdjacentTetrahedronVolume;
	} LsTmpSpringInfo;
	typedef std::unordered_map<LsHashedBCCLatticeVertex, LsTmpSpringInfo> LsTmpSpringMeshNodeInfo;
	std::unordered_map<LsHashedBCCLatticeVertex, LsTmpSpringMeshNodeInfo> temporarySpringMesh;
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
    void OnTetrahedronEmitted(const LsBCCLattice& lattice, const LsBCCLatticeRef& ref1, const LsBCCLatticeRef& ref2, const LsBCCLatticeRef& ref3, const LsBCCLatticeRef& ref4);

    // @ LsSpringMesh LsSpringMeshBuilder::Build()
	//-------------------------------------------------------------------------------
    // Process intermediate buffers and produce a spring mesh.
    //-------------------------------------------------------------------------------
    void Build();
};

