LUSTRIOUS PAINT
===============

An digital painting application that allows physics based digital painting.

In traditional painting rendering comes as a final process that is performed manually only after the form is established. Here rendering is continious feedback process and the artist works only with form and material properties.

CANVAS
------
Each pixel in the canvas represents a point in 3D space storing following information:

1. Position in space (represented by distance along the ray between canvas point and camera center)
2. Surface Normal
3. Material (as BRDF function parametrized by surface-specific properties)
4. Light Rig (which emitters and shadow casters light the pixel, painted with light strokes)

The BRDF function contains information about color and surface properties that will be used to determine final light output towards the canvas pixel.

Pixel color is determined by tracing from the pixel's position to the emitters of its light rig, see RENDERING.

### Position storage

A pixel stores its position as the distance t from the canvas camera centre along the unit length ray through the pixel centre, and the world position is reconstructed when needed as `camera position + ray direction * t`. A distance of 0 means the pixel has no position yet.

1. A point can't leave its ray, so it always projects back onto its own pixel. Brushes can't move a point to another pixel by accident or through numerical drift.
2. One fp32 value per pixel instead of three (256 MB instead of 768 MB at 8000x8000).
3. Brushes work on one value: transferring from an SDF takes the hit distance of the ray march as it is, and smoothing, flattening or pushing/pulling depth (question 21) blend distances of neighbouring pixels without drifting off the rays.
4. fp32 and linear, not a hyperbolic z-buffer depth, since shadow rays start from these positions.

Positions are tied to the canvas camera: moving the camera or changing its field of view moves every painted point with it. The painting keeps its pixels and only its relation to the scene changes, see question 15.

SCENE
-----
Collection of geometrical primitives used to influence scene geoemetry during painting. The scene geometry is specified using SDF functions.

1. Shadow Caster
2. Solid
3. Light Emitter
4. Participating Media

User may navigate in 3D space by moving camera. The projected painting, however, is attached to a single camera view. The camera and the virtual canvas will appear in 3D space showing canvas position relative to scene.

### Modeling workflow

Scene modeling borrows its workflow from Adobe Project Neo (now Illustrator 3D), which also builds 3D scenes for 2D artwork out of SDF primitives. The goal is that blocking out construction geometry feels like illustration rather than 3D modeling: a few simple shapes, combined freely, seen from the canvas camera.

What is taken from it:

1. Shapes are parametric SDF primitives, created directly in the 3D view and edited with handles and gizmos.
2. Shapes combine with boolean operations (union, subtraction, intersection), each with an adjustable smooth blend so forms merge organically instead of meeting at hard seams.
3. Combining stays non-destructive: every shape remains editable, reorderable and removable after it has been blended into others.

The details are still open, see Modeling under OPEN QUESTIONS.

BRUSH
-----
A brush is used to transfer various pixel attributes onto canvas pixels. A typical workflow is passing SDF primitive in the scene and painting its position and normals into canvas. Then setting up a light rig, painting it onto the canvas with a light stroke, and painting BRDF information with color and surface properties. The light rig contains information about which occluders and emitters are enabled per pixel, see LIGHT RIG.

Construction geometry serves as a general layout guide for large shapes while individual brush strokes can provide local attribute overrides to accentuate high-detail features in an image without having to resort to complex 3D sculpting. An artist may layout a head as a oval shape and then alter normals to create detailed face.

LIGHT RIG
---------
A light rig is a set of emitters and shadow casters that are switched on. The artist selects which lights and casters are on, or picks a saved preset, and paints the rig onto canvas pixels with a light stroke. Each pixel stores the rig it was painted with, so different regions of the canvas can be lit by different rigs (a rim light only one figure sees, a key light used only on the background), which is how painters light anyway, while each rig on its own stays physically correct.
[LsUtility](Libraries/LsUtility)
Emitters and casters live in the scene, so moving or editing one changes every pixel whose rig includes it.

RENDERING
---------
Each pixel of physically based digital painting is rendered using raytracing. The entire hemisphere of rays must be traced for each voxel corresponding to pixel on canvas, against the emitters and shadow casters of the pixel's light rig. Secondary bounces need not be considered, only tracing to emitters and checking for shadow casters.

A pixel is re-rendered when its own attributes change (position, normal, BRDF, light rig) or when an emitter or shadow caster in its rig is changed. Changes in scene geometry which is not part of any light rig, but that was used as construction geometry to set base voxel positions, will not trigger a re-render, unless their positions are transferred onto the voxels.

REFLECTIONS (REVERSE LIGHT TRACING)
-----------------------------------
Painting complex reflections is hard with light rigs alone. If an object is highly reflective and the artist wants a reflected scene visible in it (a window, a landscape, a room), building that out of emitters would require modelling the whole reflected environment.

Instead the artist paints the reflection directly onto the canvas, and the paint is pushed backwards into a paintable emitter: an emitter with a texture holding its emitted light. By default it surrounds the scene at infinite distance (like an environment map); a plane or box at finite distance gives parallax. It is part of the light rig like any other emitter, so the rig controls which voxels see it, and every voxel that has it enabled reflects it towards the canvas.

### Backward pass

For a fixed light rig a voxel's output is linear in the emitter texture:

    L_o = ∫ w(ω) · L_tex(hit(ω)) dω,   w = BRDF · cosθ

The brush works for any BRDF without knowing a dominant reflection direction:

1. For each voxel under the brush, trace hemisphere samples exactly as the renderer does (BRDF importance sampling).
2. For every sample that hits the paintable emitter, record the texel and its weight w/pdf.
3. Compute the residual between the brush target and the voxel's current L_o.
4. Add opacity · residual · weight / Σweight² to each hit texel.

This is the smallest texture change that moves the voxel towards the target, which makes it one step of a relaxed iterative least-squares solve (Kaczmarz). Glossy BRDFs concentrate their samples, so paint lands sharp; rough BRDFs spread them, so paint lands soft. Anisotropic, multi-lobe or measured BRDFs get whatever their samples dictate.

Reflection directions come from each voxel's own normal and view direction. With a perspective camera, two voxels with the same normal see the canvas from different directions, so they reflect different parts of the painted emitter.

### Not exact by design

All surfaces scatter, so the painted value is a target, not a constraint. Detail finer than a voxel's BRDF lobe cannot appear on that voxel, and where a match is impossible the result is the closest achievable one. Repeated dabs over many voxels converge to the best fit across everything painted.

Diffuse lobes spread the residual faintly over the whole hemisphere, which is physically correct but leaks paint into what every other voxel sees. The brush offers an option to exclude the diffuse lobe from the update (or to weight towards high-w samples) when the artist wants local edits.

### Additive strokes

The texture is never overwritten; every dab adds a delta, so overlapping strokes and voxels sharing texels combine instead of fighting. The brush target may still be absolute:

| Mode | Delta | Behaviour |
|---|---|---|
| Paint toward | α · (color − current) | Normal paint; converges on the color and stops |
| Add light | α · color | Glow/dodge; keeps brightening, no target, no forward evaluation needed |

Emitted light cannot be negative, so subtractive variants clamp texels at zero. Painting darker than the rig's other emitters already make a voxel cannot be fully achieved; such voxels should be flagged so the artist can dim or occlude an emitter instead.

Within a stroke the residual is measured against the state at the start of the stroke, so opacity caps the whole stroke and flow controls how fast the dabs approach it (as with opacity vs. flow in raster painting apps). Otherwise overlapping dabs would compound and overshoot.

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

   Constrained to the ray, stored as the distance along it, see Position storage under CANVAS. Off the ray is not allowed.

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

### Modeling

37. Which parts of the Neo workflow are needed, and which first? Candidates: smooth union / subtraction / intersection, a per-shape blend radius, rounding/bevel on every primitive, dragging shapes out on surfaces, extruding or revolving drawn 2D profiles, mirror symmetry, repeated copies.
    Answer:

38. Where does a boolean operation live? Either each shape carries an operation applied to the shapes before it in its group (an ordered stack, so outliner order matters), or a group carries one operation that combines all its children. The first is closer to how Neo-like tools read; the second is simpler to reason about.
    Answer:

39. How do booleans interact with roles? Can a construction shape cut into a shadow caster, or do shapes only combine with others that have the same roles? Should a subtracting shape have roles of its own at all?
    Answer:

40. Are custom 2D profiles needed (paths drawn in the view or imported SVG, then extruded or revolved)? That needs an SDF of polygons/Bézier paths, which is a bigger step than the fixed primitives.
    Answer:

41. How is a shape created: click to drop a default-sized shape on a surface (as the 3D view does now), or press and drag on a surface to size its footprint, then its height? Should new shapes align to the surface normal, and snap to the grid or other shapes?
    Answer:

42. Should shapes be sized by their own dimensions (width/height/depth handles) rather than by non-uniform scale? Non-uniform scale makes SDF distances inexact (the renderer can only use a conservative bound), and dimension handles are how Neo-style tools usually resize.
    Answer:

43. Do shapes carry material or colour? In Neo they do. Here shapes are construction geometry; should they hold default BRDF values that the brush can transfer together with position and normal, so the first pass over the canvas already looks like a shaded block-out?
    Answer:

44. How many shapes is a typical scene expected to have: tens, or hundreds? Smooth blends stop shapes from being culled independently, so beyond a few dozen the renderer and brush will need an acceleration structure (bounding volume hierarchy or a baked distance volume).
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

    For a first pass, the brush sphere traces the construction geometry from the canvas camera through each covered pixel centre and writes the distance of the first hit. Pixels whose ray misses keep what they had.

19. Can a stroke write a selected subset of channels (e.g. only normal, only roughness, only light-rig mask) - i.e. channel locks/masks per brush?
    Answer:

20. How do soft brush edges / opacity blend non-colour attributes? Lerp positions and re-normalise (slerp) normals? Should pen pressure / tilt (SDL pen events) map to size, opacity, or other parameters?
    Answer:

21. Can positions/normals be painted freehand without an SDF (e.g. push/pull depth, normal "smudge", flatten, smooth brushes)?
    Answer:

    A smooth brush should be implemented and mapped to hotkey such as Shift, similarly to sculpting applications it is expected to be used frequently to blend surfaces.

22. Light rig per pixel "contains information about which occluders and emitters are enabled". How many emitters/occluders must be addressable (a 32-bit mask? 64?). Are light-rig masks painted with the same brush, and can there be named light rigs/presets?
    Answer:

    The artist selects which lights are on, or picks a preset, and paints with it in a light stroke (see LIGHT RIG). The mask size and how soft brush edges blend two rigs are still open, see question 45.

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

### Reflections

32. How is the painted light field stored: a per-rig environment map (cube map / octahedral map), spherical harmonics, or a sparse set of painted directional samples? Its resolution limits how sharp a mirror reflection can be.
    Answer:

33. For rough BRDFs one painted value constrains an integral over many directions, so the inversion is underdetermined. Distribute it evenly over the lobe, weight by the BRDF, or only allow reverse painting on voxels with roughness below a threshold?
    Answer:

    Weight by the BRDF, for any BRDF: the backward pass pushes the residual along the renderer's own hemisphere samples, proportional to w/pdf (see REFLECTIONS). The painted value is a target, not an exact constraint; rough voxels get a soft, closest-achievable result. Diffuse lobes can optionally be excluded from the update.

34. When strokes on different voxels constrain the same directions with different values, which wins: the last stroke, an average / least-squares fit, or should the conflict be shown to the artist?
    Answer:

    Least-squares fit. Strokes add deltas to the emitter texture instead of setting values, and each dab is a relaxed Kaczmarz step, so conflicting strokes converge to the best compromise. Voxels that cannot reach their target (e.g. need negative light) should be flagged.

35. Is the painted light field occluded by the rig's shadow casters when lighting other voxels, like any other emitter at infinity?
    Answer:

36. With a perspective camera the view direction changes across the canvas, so the same normal reflects slightly different directions at different pixels. Is the infinite-distance assumption acceptable here, or should painted light optionally be given a finite distance (e.g. projected onto a sphere/box proxy) for parallax?
    Answer:

    Correct, the same normal does not give the same reflection under perspective, so the reflected directions must be recomputed per voxel from its own view direction. Painted light stays at infinite distance; only the lookup direction changes.

### Light rig

45. How does a soft brush edge or partial opacity blend two rigs? Options: the pixel stores one rig (hard edges only); it stores a weight per light (a partly-on light); or it stores up to two rigs with a blend weight.
    Answer:

46. Parked: storing captured incident light per pixel instead of a rig, which would decouple the canvas from the scene so pixels change only when painted. Set aside because a sharp per-pixel light environment is too large (gigabytes at 8000x8000 even compressed); the most compact form found was storing frozen emitter snapshots plus per-pixel visibility.
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



