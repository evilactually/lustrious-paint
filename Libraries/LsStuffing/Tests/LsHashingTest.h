
#include "LsBCCLatticeTypes.h"
#include "LsLatticeVertexRef.h"













TEST_CASE( "LsLatticeVertexRef Basic Hashing", "[stuffing]" ) {
    LsLatticeVertexRef ref1 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(2,0,0));
    LsLatticeVertexRef ref2 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(2,0,0));
    REQUIRE ( std::hash<LsLatticeVertexRef>()(ref1) == std::hash<LsLatticeVertexRef>()(ref2) );

    ref1 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,1,0));
    ref2 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,1,0));
    REQUIRE ( std::hash<LsLatticeVertexRef>()(ref1) == std::hash<LsLatticeVertexRef>()(ref2) );

    ref1 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,0,5));
    ref2 = LsLatticeVertexRef::MkNodeRef(NULL, LsBCCNode(0,0,5));
    REQUIRE ( std::hash<LsLatticeVertexRef>()(ref1) == std::hash<LsLatticeVertexRef>()(ref2) );
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
        REQUIRE ( std::hash<LsLatticeVertexRef>()(ref1) == std::hash<LsLatticeVertexRef>()(ref2) );
        REQUIRE ( std::hash<LsLatticeVertexRef>()(ref1) != std::hash<LsLatticeVertexRef>()(ref3) );
        REQUIRE ( std::hash<LsLatticeVertexRef>()(ref1) != std::hash<LsLatticeVertexRef>()(ref4) );
        REQUIRE ( std::hash<LsLatticeVertexRef>()(ref1) != std::hash<LsLatticeVertexRef>()(ref5) );
    }
}