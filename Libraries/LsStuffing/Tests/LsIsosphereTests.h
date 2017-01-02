#include <LsBCCLatticeTypes.h>
#include <LsIsosphere.h>

#include <LsMath.h>

TEST_CASE( "sphere01" ) {
  LsIsosphere isosphere({0.0f,0.0f,0.0f}, 2.0f);
  // Domain check
  LsDomain domain = isosphere.GetDomain();
  REQUIRE((domain.x2 - domain.x1) == 4.0f);
  REQUIRE((domain.y2 - domain.y1) == 4.0f);
  REQUIRE((domain.z2 - domain.z1) == 4.0f);
  // In/Out check
  REQUIRE( isosphere.EvaluateAt({0.0f,0.0f,0.0f}) == LsBCCValue::ePositive );
  REQUIRE( isosphere.EvaluateAt({0.0f,0.0f,1.9f}) == LsBCCValue::ePositive );
  REQUIRE( isosphere.EvaluateAt({0.0f,0.0f,-1.9f}) == LsBCCValue::ePositive );
  REQUIRE( isosphere.EvaluateAt({0.0f,1.9f,0.0f}) == LsBCCValue::ePositive );
  REQUIRE( isosphere.EvaluateAt({0.0f,-1.9f,0.0f}) == LsBCCValue::ePositive );
  REQUIRE( isosphere.EvaluateAt({1.9f,0.0f,0.0f}) == LsBCCValue::ePositive );
  REQUIRE( isosphere.EvaluateAt({-1.9f,0.0f,0.0f}) == LsBCCValue::ePositive );
  REQUIRE( isosphere.EvaluateAt({0.0f,0.0f,2.0f}) == LsBCCValue::eZero );
  REQUIRE( isosphere.EvaluateAt({0.0f,0.0f,2.1f}) == LsBCCValue::eNegative );
  // Intersect check
  REQUIRE(glm::all(glm::equal(isosphere.IntersectByEdge({0.0f,0.0f,0.0f}, {0.0f,0.0f,10.0f}), {0.0f,0.0f,2.0f})));
  auto v = isosphere.IntersectByEdge({0.0f,0.0f,1.0f}, {0.0f,0.0f,10.0f});
  REQUIRE(glm::all(glm::equal(isosphere.IntersectByEdge({0.0f,0.0f,1.0f}, {0.0f,0.0f,10.0f}), {0.0f,0.0f,2.0f})));
  REQUIRE(glm::all(glm::equal(isosphere.IntersectByEdge({0.0f,0.0f,10.0f}, {0.0f,0.0f,1.0f}), {0.0f,0.0f,2.0f})));
  REQUIRE(glm::all(glm::equal(isosphere.IntersectByEdge({0.0f,0.0f,-1.0f}, {0.0f,0.0f,10.0f}), {0.0f,0.0f,2.0f})));
  REQUIRE(IvAreEqual(glm::length(isosphere.IntersectByEdge({0.0f,0.0f,0.0f}, {10.0f,10.0f,10.0f})), 2.0f));
  //REQUIRE(IvAreEqual(glm::length(isosphere.IntersectByEdge({1.0f,1.0f,1.0f}, {10.0f,10.0f,10.0f})), 2.0f));
  //REQUIRE(IvAreEqual(glm::length(isosphere.IntersectByEdge({-1.0f,-1.0f,-1.0f}, {10.0f,10.0f,10.0f})), 2.0f));
  REQUIRE(glm::all(glm::equal(isosphere.IntersectByEdge({0.0f,0.0f,0.0f}, {0.0f,0.0f,10.0f}), {0.0f,0.0f,2.0f})));
}