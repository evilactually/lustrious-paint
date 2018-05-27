#pragma once

class LsLatticeVertexRef;

class LsITetrahedronProcessor
{
public:
	virtual void OnTetrahedronEmitted(const LsLatticeVertexRef& ref1, const LsLatticeVertexRef& ref2, const LsLatticeVertexRef& ref3, const LsLatticeVertexRef& ref4) = 0;
};