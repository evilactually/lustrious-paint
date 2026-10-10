// Surface reflectance of canvas points, see RENDERING in SPECIFICATION.md.
// Import with `import brdf;`.
//
// The metallic-roughness model of glTF 2.0 (Appendix B of its specification):
// a Lambertian base under a dielectric Trowbridge-Reitz (GGX) coat for
// non-metals, the coat alone tinted by the base colour for metals, blended by
// metallic. The microfacet distribution, Smith masking and visible normal
// sampling follow pbrt-v4's TrowbridgeReitzDistribution, isotropic here.
//
// Directions are in the local shading frame, the normal is +z, and both wo
// (towards the viewer) and wi (towards the light) point away from the surface.

static const float PI = 3.14159265358979;

struct Brdf
{
  float3 baseColor; // linear
  float metallic;
  float alpha;      // Trowbridge-Reitz roughness, the square of perceptual roughness
  float ior;        // of the dielectric coat
};

// Trowbridge-Reitz distribution of microfacet normals
float distributionD(float alpha, float3 wm)
{
  float cos2Theta = wm.z*wm.z;
  if (cos2Theta < 1.0e-8) {
    return 0.0;
  }
  float alpha2 = alpha*alpha;
  float e = (wm.x*wm.x + wm.y*wm.y)/alpha2 + cos2Theta;
  return 1.0/(PI*alpha2*e*e);
}

// Smith's auxiliary function
float lambda(float alpha, float3 w)
{
  float cos2Theta = w.z*w.z;
  if (cos2Theta < 1.0e-8) {
    return 1.0e8;
  }
  float tan2Theta = max(1.0 - cos2Theta, 0.0)/cos2Theta;
  return (sqrt(1.0 + alpha*alpha*tan2Theta) - 1.0)*0.5;
}

float maskingG1(float alpha, float3 w)
{
  return 1.0/(1.0 + lambda(alpha, w));
}

// Height correlated masking and shadowing
float maskingG(float alpha, float3 wo, float3 wi)
{
  return 1.0/(1.0 + lambda(alpha, wo) + lambda(alpha, wi));
}

// Microfacet normal sampled from the normals visible from w, pbrt-v4's Sample_wm
float3 sampleVisibleNormal(float alpha, float3 w, float2 u)
{
  // Stretched to the hemisphere configuration
  float3 wh = normalize(float3(alpha*w.x, alpha*w.y, w.z));
  if (wh.z < 0.0) {
    wh = -wh;
  }
  float3 t1 = wh.z < 0.99999 ? normalize(cross(float3(0.0, 0.0, 1.0), wh)) : float3(1.0, 0.0, 0.0);
  float3 t2 = cross(wh, t1);

  // Uniform on the disk, warped to the projection of the visible hemisphere
  float r = sqrt(u.x);
  float phi = 2.0*PI*u.y;
  float2 p = float2(r*cos(phi), r*sin(phi));
  float h = sqrt(1.0 - p.x*p.x);
  p.y = lerp(h, p.y, (1.0 + wh.z)*0.5);

  float pz = sqrt(max(0.0, 1.0 - dot(p, p)));
  float3 nh = p.x*t1 + p.y*t2 + pz*wh;
  return normalize(float3(alpha*nh.x, alpha*nh.y, max(1.0e-6, nh.z)));
}

// Density of sampleVisibleNormal followed by reflection about the normal, as a
// density of wi
float specularPdf(float alpha, float3 wo, float3 wi)
{
  float3 wm = wo + wi;
  if (dot(wm, wm) < 1.0e-12 || wo.z <= 0.0) {
    return 0.0;
  }
  wm = normalize(wm);
  // Visible normal density G1(wo)/cos(wo)*D(wm)*|wo.wm| times the Jacobian 1/(4|wo.wm|)
  return maskingG1(alpha, wo)*distributionD(alpha, wm)/(4.0*wo.z);
}

float3 schlickFresnel(float3 f0, float cosTheta)
{
  float m = saturate(1.0 - cosTheta);
  float m2 = m*m;
  return f0 + (1.0 - f0)*(m2*m2*m);
}

// Reflectance at normal incidence of a dielectric of this index in air
float dielectricF0(float ior)
{
  float r = (ior - 1.0)/(ior + 1.0);
  return r*r;
}

float3 evaluateBrdf(Brdf brdf, float3 wo, float3 wi)
{
  if (wo.z <= 0.0 || wi.z <= 0.0) {
    return float3(0.0);
  }
  float3 wm = normalize(wo + wi);
  float cosHalf = saturate(dot(wo, wm));
  float specular = distributionD(brdf.alpha, wm)*maskingG(brdf.alpha, wo, wi)/(4.0*wo.z*wi.z);

  // The dielectric coat reflects F and lets the rest through to the diffuse base
  float coatFresnel = schlickFresnel(float3(dielectricF0(brdf.ior)), cosHalf).x;
  float3 dielectric = lerp(brdf.baseColor/PI, float3(specular), coatFresnel);
  float3 metal = schlickFresnel(brdf.baseColor, cosHalf)*specular;
  return lerp(dielectric, metal, brdf.metallic);
}

// How often sampleBrdf picks the specular lobe. Only depends on wo, so the
// density of a direction is known whichever lobe produced it.
float specularProbability(Brdf brdf, float3 wo)
{
  float coat = schlickFresnel(float3(dielectricF0(brdf.ior)), wo.z).x;
  return clamp(lerp(coat, 1.0, brdf.metallic), 0.1, 0.9);
}

// Density of sampleBrdf over wi
float brdfPdf(Brdf brdf, float3 wo, float3 wi)
{
  if (wo.z <= 0.0 || wi.z <= 0.0) {
    return 0.0;
  }
  float p = specularProbability(brdf, wo);
  return p*specularPdf(brdf.alpha, wo, wi) + (1.0 - p)*wi.z/PI;
}

// A direction distributed like the BRDF times the cosine: visible normals for
// the specular lobe, cosine weighted for the diffuse one. u.z picks the lobe.
float3 sampleBrdf(Brdf brdf, float3 wo, float3 u)
{
  if (u.z < specularProbability(brdf, wo)) {
    float3 wm = sampleVisibleNormal(brdf.alpha, wo, u.xy);
    return reflect(-wo, wm);
  }
  float r = sqrt(u.x);
  float phi = 2.0*PI*u.y;
  return float3(r*cos(phi), r*sin(phi), sqrt(max(0.0, 1.0 - u.x)));
}
