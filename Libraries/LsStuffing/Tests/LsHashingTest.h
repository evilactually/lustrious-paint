
#include "LsBCCLatticeTypes.h"
#include "LsLatticeVertexRef.h"













TEST_CASE( "LsLatticeVertexRef Basic Hashing", "[stuffing]" ) {
	LsLatticeVertexRef ref1 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(2,0,0));
	LsLatticeVertexRef ref2 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(2,0,0));
	REQUIRE ( ref1.GetHash() == ref2.GetHash() );

	ref1 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,1,0));
	ref2 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,1,0));
	REQUIRE ( ref1.GetHash() == ref2.GetHash() );

	ref1 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,0,5));
	ref2 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,0,5));
	REQUIRE ( ref1.GetHash() == ref2.GetHash() );
}

TEST_CASE( "LsLatticeVertexRef Hash Annihilation", "[stuffing]" ) {
	for (int i = 0; i < 1000; ++i)
	{
		int x = rand();
		int y = rand();
		int z = rand();
		LsLatticeVertexRef ref1 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(x,y,z));
     	LsLatticeVertexRef ref2 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(x,y,z));
     	LsLatticeVertexRef ref3 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(x+rand(),y,z));
     	LsLatticeVertexRef ref4 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(x,y+rand(),z));
     	LsLatticeVertexRef ref5 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(x,y,z+rand()));
	    REQUIRE ( ref1.GetHash() == ref2.GetHash() );
	    REQUIRE ( ref1.GetHash() != ref3.GetHash() );
	    REQUIRE ( ref1.GetHash() != ref4.GetHash() );
	    REQUIRE ( ref1.GetHash() != ref5.GetHash() );
	}
}