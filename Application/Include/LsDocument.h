#pragma once

#include <string>
#include <utility>
#include <vector>
#include <LsCanvas.h>
#include <LsScene.h>

//-------------------------------------------------------------------------------
// Saving a painting to a .lspaint file and opening it again
//-------------------------------------------------------------------------------
// The file is little endian binary: the magic "LSPAINT\0", a uint32 version,
// then chunks of a four character tag, a uint64 size and that many bytes, so
// readers can skip chunks they don't know. Strings are a uint32 length and
// their bytes, vectors their floats in order.
//
//   SCEN  The scene tree, nodes in depth first order from the root, each with
//         the index of its parent, and the index of the active camera
//   CANV  The canvas size and its attribute images, see LsCanvasAttributes
//   LITE  Local lights with their slots, which the rigs refer to
//   OCCL  Occluders with their slots
//   RIGS  Light rigs with their slots, which the canvas pixels refer to
//   BRDF  The brush BRDF and the BRDFs painted so far
//   REND  Exposure and samples of the rendering
//
// Rendered samples are left out, they are rendered again.
//-------------------------------------------------------------------------------

const char* const kLsDocumentExtension = "lspaint";

// Writes the document, false with a message in error when it can't
bool LsSaveDocument(const std::string& path, const LsScene& scene, const LsCanvas& canvas,
                    const LsCanvasAttributes& attributes, std::string& error);

// Everything a document holds, as read from a file
struct LsDocument
{
  LsScene scene;
  LsCanvasAttributes attributes;
  // With the slots they had, which the rigs and the canvas pixels refer to
  std::vector<std::pair<uint32_t, LsLocalLight>> lights;
  std::vector<std::pair<uint32_t, LsOccluder>> occluders;
  std::vector<std::pair<uint32_t, LsLightRig>> rigs;
  LsBrdfParameters brushBrdf;
  std::vector<LsBrdfParameters> paintedBrdfs;
  LsCanvasRenderSettings renderSettings;
};

// Reads a document, false with a message in error when the file can't be read
// or isn't one. Chunks it doesn't know are skipped.
bool LsLoadDocument(const std::string& path, LsDocument& document, std::string& error);
