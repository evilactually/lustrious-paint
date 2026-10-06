LUSTRIOUS PAINT
===============

An digital painting application that allows physics based digital painting.

In traditional painting rendering comes as a final process that is performed manually only after the form is established. Here rendering is continious feedback process and the artist works only with form and material properties.

CANVAS
------
Each pixel in the canvas represents a paint in 3D space.

1. World Position
2. Absolute Normal
3. BRDF function

The BRDF function contains information about color and surface properties that will be used to determine final light output towards the canvas pixel.

Pixel color is determined by performing a single path trace to scene light sources.

SCENE
-----
Collection of geometrical primitives used to influence scene geoemetry during painting. The scene geometry is specified using SDF functions.

1. Shadow Caster
2. Surface Normal
3. Light Emitter
4. Participating Media

User may navigate in 3D space by moving camera. The projected painting, however, is attached to a single camera view. The camera and the virtual canvas will appear in 3D space showing canvas position relative to scene.

BRUSH
-----
A brush is used to transfer various pixel attributes onto canvas pixels. A typical workflow is passing SDF primitive in the scene and painting its position and normals into canvas. Then setting up a light rig and painting BRDF information with color and surface properties. The light rig simply contains information about which occluders and emitters are enabled per pixel.

Construction geometry serves as a general layout guide for large shapes while individual brush strokes can provide local attribute overrides to accentuate high-detail features in an image without having to resort to complex 3D sculpting. An artist may layout a head as a oval shape and then alter normals to create detailed face.

RENDERING
---------
Each pixel of physically based digital painting is rendered using raytracing. The entire hemisphere of rays must be traced for each voxel corresponding to pixel on canvas. Secondary bounces need not be considred, only tracing to emitters and checking for shadow casters.

Changes in light rig will trigger rerendering, same as change in voxel properties, such as BRDF or normal changes. Changes in sccene geoemtry which is not part of light rig, but that was used as construction geometry to set base voxel positions will not trigger rerender, unless their positions are transferred onto the voxels.

OPEN QUESTIONS
--------------
Questions raised while reviewing this specification. Fill in the answer under each question; resolved answers should eventually be folded into the sections above.

### Terminology

1. "Pixel" and "voxel" are both used for a canvas element (CANVAS vs RENDERING). Are they the same thing (one canvas pixel = one 3D surface point), or is there a separate voxel volume?
   Answer:

2. CANVAS says "Pixel color is determined by performing a single path trace to scene light sources", while RENDERING says "the entire hemisphere of rays must be traced". Which is it: one shadow ray per light (direct lighting only, deterministic), or Monte Carlo hemisphere sampling accumulated progressively over frames? If progressive, how many samples count as "converged"?
   Answer:

Since some light can be area lights (SDF geometric objects as emitters) we need to integrate light contributions from all directions, so it needs to be hemisphere sampling. The good news we do not need to consider secondary bounces.

### Canvas

3. What does "Absolute Normal" mean: a world-space normal (as opposed to camera/tangent-space)? Is it stored per pixel independently of position, i.e. can a painted normal disagree with the geometry implied by neighbouring positions?
   Answer:

4. Is World Position free 3D, or constrained to lie on the canvas camera ray through the pixel (so it can be stored as a single depth value)? Painting a position off the ray would make the point project to a different pixel - is that allowed?
   Answer:

5. What happens with pixels that have no position yet (never painted)? Background colour, environment light, transparent, excluded from rendering?
   Answer: They will show microwave background radioation from Big Bang. No, they are just black.

6. What is the BRDF representation? Options: a fixed parametric model (e.g. GGX base colour / metallic / roughness / specular), a selectable set of models, or something more general (tabulated, layered). Do we need emission, transmission/subsurface, or clear-coat per pixel?
   Answer:

   I did not consider emissive voxels yet.

7. Target canvas resolution and precision? The current code allocates 8000x8000 images - is that the expected maximum? Precision for position (fp32 vs fp16/depth), normals (octahedral 16-bit?), colour (fp16 HDR?). This drives memory per pixel.
   Answer:

8. Are there layers on the canvas (with per-layer attributes and blend modes), or a single flat attribute buffer?
   Answer:

   First implementation single layer.

9. Do painted canvas points cast shadows on each other (self-shadowing of the painting), or only scene Shadow Casters occlude light?
   Answer:

No, painted points do not cas shadows. The artist places shadow casters in the scene when he needs them as part of light rig.

### Scene

10. Which SDF primitives are required (sphere, box, capsule, torus, plane, cylinder, ...)? Are CSG operations (union, subtraction, intersection, smooth blends) and arbitrary transforms (translate/rotate/non-uniform scale) needed? Imported meshes converted to SDF?
    Answer:

    SDF are chosen to make CSG easier. Importing custom meshes not planned in short-term. If using custom meshes they would have to work in SDF space.

11. Are the four roles (Shadow Caster, Surface Normal, Light Emitter, Participating Media) mutually exclusive per primitive, or flags that can be combined (e.g. a sphere that is both construction geometry and a shadow caster)? What exactly is the "Surface Normal" role - construction geometry that the brush samples position/normal from?
    Answer:

12. Light emitters: are they only SDF-shaped area lights, or also point / spot / directional / environment (HDRI) lights? What parameters - colour, intensity units, falloff, temperature?
    Answer:

13. Participating media: homogeneous only, or heterogeneous (noise/density fields)? With "no secondary bounces", does media only attenuate shadow rays (transmittance), or should single-scattering in-scatter (e.g. visible light shafts in front of the canvas) also be rendered? Does the camera-to-pixel segment pass through media?
    Answer:

14. How is the scene edited - 3D gizmos in the viewport, numeric property panels, or both? Is there an outliner/list of primitives?
    Answer:

### Camera and views

15. Is the canvas camera perspective or orthographic? Can FOV / position be changed after painting has started (and what happens to existing pixels - reprojection, or the canvas is locked)?
    Answer:

16. One canvas per document, or several canvases/camera views in the same scene?
    Answer:

17. When navigating away from the canvas camera, should the painted points be shown as a 3D point cloud/surface in the scene view, or only the canvas frame as a quad?
    Answer:

### Brush

18. How does the brush transfer position/normal from an SDF: ray-march from the canvas camera through each covered pixel and take the first hit? What about pixels the brush covers where the ray misses the SDF?
    Answer:

19. Can a stroke write a selected subset of channels (e.g. only normal, only roughness, only light-rig mask) - i.e. channel locks/masks per brush?
    Answer:

20. How do soft brush edges / opacity blend non-colour attributes? Lerp positions and re-normalise (slerp) normals? Should pen pressure / tilt (SDL pen events) map to size, opacity, or other parameters?
    Answer:

21. Can positions/normals be painted freehand without an SDF (e.g. push/pull depth, normal "smudge", flatten, smooth brushes)?
    Answer:

    A smooth brush should be implemented and mapped to hotkey such as Shift, similarly to sculpting applications it is expected to be used frequently to blend surfaces.

22. Light rig per pixel "contains information about which occluders and emitters are enabled". How many emitters/occluders must be addressable (a 32-bit mask? 64?). Are light-rig masks painted with the same brush, and can there be named light rigs/presets?
    Answer:

23. Colour picking: in which colour space is albedo painted (sRGB picker, linear storage)? Is there an eyedropper that picks rendered colour vs. stored albedo?
    Answer:

Stored attribute should be the default choice for picking.

### Rendering

24. Expected interactivity: should rendering update in real time while painting (e.g. only re-render dirty pixels/rectangles touched by the stroke), or is an explicit/background render acceptable?
    Answer:

25. Implementation approach: SDF sphere tracing in Vulkan compute shaders (current direction with Shaders/paint.comp), or hardware ray tracing (VK_KHR_ray_query) where available? Minimum GPU/Vulkan version to support?
    Answer:

26. Output pipeline: HDR accumulation with tone mapping / exposure controls? Which export formats (PNG, EXR, layered PSD) and should raw attribute buffers (position/normal/BRDF) be exportable?
    Answer:

27. Are emitters and shadow casters themselves visible in the final painting, or is the output strictly the canvas pixels?
    Answer:

### Application

28. Document persistence: what file format stores canvas attributes + scene + camera? Is undo/redo required, and for which operations (strokes, scene edits, both)?
    Answer:

29. Target platforms: Windows only, or cross-platform (the move to SDL3 suggests Linux/macOS too)? Tablet support beyond SDL pen events?
    Answer:

30. What happens to the existing demo components (LsBrushRig, LsPointGrid, LsCanvasPainter, LsStuffingDemo, isosurface stuffing / tetrahedron mesh code)? Are they part of the product, prototypes to keep, or to be removed?
    Answer:

31. What is the first milestone (MVP)? Suggested: single canvas, sphere/box SDFs, position+normal transfer brush, one parametric BRDF, point/area lights with hard shadows, progressive render of dirty pixels. Is that the right scope?
    Answer:



