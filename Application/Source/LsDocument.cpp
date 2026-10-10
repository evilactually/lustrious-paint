#include <LsDocument.h>

#include <cstdio>
#include <cstring>
#include <vector>

static const uint32_t kDocumentVersion = 1;

// Appends values to a byte buffer in the file's little endian layout
class LsDocumentWriter
{
public:
  std::vector<uint8_t> bytes;

  void Raw(const void* data, size_t size) {
    const uint8_t* begin = static_cast<const uint8_t*>(data);
    bytes.insert(bytes.end(), begin, begin + size);
  }
  void U8(uint8_t value) { bytes.push_back(value); }
  void U32(uint32_t value) { Raw(&value, sizeof(value)); }
  void U64(uint64_t value) { Raw(&value, sizeof(value)); }
  void F32(float value) { Raw(&value, sizeof(value)); }
  void Vec3(glm::vec3 v) { F32(v.x); F32(v.y); F32(v.z); }
  void Vec4(glm::vec4 v) { F32(v.x); F32(v.y); F32(v.z); F32(v.w); }
  void String(const std::string& s) {
    U32(static_cast<uint32_t>(s.size()));
    Raw(s.data(), s.size());
  }
  // A tagged chunk with the bytes another writer collected
  void Chunk(const char tag[4], const LsDocumentWriter& chunk) {
    Raw(tag, 4);
    U64(chunk.bytes.size());
    Raw(chunk.bytes.data(), chunk.bytes.size());
  }
};

static void WriteNode(LsDocumentWriter& out, const LsScene& scene, LsNodeId id, uint32_t parentIndex,
                      uint32_t& nextIndex, uint32_t& activeCameraIndex) {
  const LsSceneNode* node = scene.Find(id);
  const LsSceneNodeProperties& properties = node->GetProperties();
  uint32_t index = nextIndex++;
  if (id == scene.GetActiveCamera()) {
    activeCameraIndex = index;
  }
  out.U32(parentIndex);
  out.String(properties.name);
  out.U32(static_cast<uint32_t>(properties.type));
  out.U8(properties.visible ? 1 : 0);
  out.Vec3(properties.placement.translation);
  out.Vec3(properties.placement.rotation);
  out.Vec3(properties.placement.scale);
  out.U32(static_cast<uint32_t>(properties.shape.type));
  out.Vec4(properties.shape.params);
  out.U32(properties.roles);
  out.F32(properties.lens.verticalFov);
  out.Vec3(properties.emissionColor);
  out.F32(properties.emissionIntensity);
  out.Vec3(properties.mediumAlbedo);
  out.F32(properties.mediumDensity);
  for (LsNodeId child : node->GetChildren()) {
    WriteNode(out, scene, child, index, nextIndex, activeCameraIndex);
  }
}

static void WriteBrdf(LsDocumentWriter& out, const LsBrdfParameters& brdf) {
  out.Vec3(brdf.baseColor);
  out.F32(brdf.metallic);
  out.F32(brdf.roughness);
  out.F32(brdf.ior);
}

bool LsSaveDocument(const std::string& path, const LsScene& scene, const LsCanvas& canvas,
                    const LsCanvasAttributes& attributes, std::string& error) {
  LsDocumentWriter file;
  file.Raw("LSPAINT", 8); // with its terminating zero
  file.U32(kDocumentVersion);

  // The tree is written after its node count, which isn't known until then
  LsDocumentWriter nodes;
  uint32_t nodeCount = 0;
  uint32_t activeCameraIndex = UINT32_MAX;
  WriteNode(nodes, scene, scene.GetRoot(), UINT32_MAX, nodeCount, activeCameraIndex);
  LsDocumentWriter sceneChunk;
  sceneChunk.U32(activeCameraIndex);
  sceneChunk.U32(nodeCount);
  sceneChunk.Raw(nodes.bytes.data(), nodes.bytes.size());
  file.Chunk("SCEN", sceneChunk);

  LsDocumentWriter canvasChunk;
  canvasChunk.U32(attributes.width);
  canvasChunk.U32(attributes.height);
  canvasChunk.Raw(attributes.distance.data(), attributes.distance.size()*sizeof(float));
  canvasChunk.Raw(attributes.normal.data(), attributes.normal.size()*sizeof(uint32_t));
  canvasChunk.Raw(attributes.materialColor.data(), attributes.materialColor.size()*sizeof(uint32_t));
  canvasChunk.Raw(attributes.materialSurface.data(), attributes.materialSurface.size()*sizeof(uint32_t));
  canvasChunk.Raw(attributes.rig.data(), attributes.rig.size()*sizeof(uint32_t));
  file.Chunk("CANV", canvasChunk);

  LsDocumentWriter lights;
  uint32_t lightCount = 0;
  LsDocumentWriter lightEntries;
  for (uint32_t slot = 0; slot < kLsMaxLocalLights; slot++) {
    if (const LsLocalLight* light = canvas.GetLocalLight(slot)) {
      lightEntries.U32(slot);
      lightEntries.String(light->name);
      lightEntries.Vec3(light->direction);
      lightEntries.Vec3(light->color);
      lightEntries.F32(light->intensity);
      lightEntries.F32(light->size);
      lightEntries.U8(light->shadows ? 1 : 0);
      lightCount++;
    }
  }
  lights.U32(lightCount);
  lights.Raw(lightEntries.bytes.data(), lightEntries.bytes.size());
  file.Chunk("LITE", lights);

  LsDocumentWriter occluders;
  uint32_t occluderCount = 0;
  LsDocumentWriter occluderEntries;
  for (uint32_t slot = 0; slot < kLsMaxOccluders; slot++) {
    if (const LsOccluder* occluder = canvas.GetOccluder(slot)) {
      occluderEntries.U32(slot);
      occluderEntries.String(occluder->name);
      occluderEntries.Vec3(occluder->direction);
      occluderEntries.F32(occluder->size);
      occluderEntries.F32(occluder->softness);
      occluderEntries.F32(occluder->strength);
      occluderCount++;
    }
  }
  occluders.U32(occluderCount);
  occluders.Raw(occluderEntries.bytes.data(), occluderEntries.bytes.size());
  file.Chunk("OCCL", occluders);

  LsDocumentWriter rigs;
  uint32_t rigCount = 0;
  LsDocumentWriter rigEntries;
  for (uint32_t slot = 1; slot < kLsMaxRigs; slot++) {
    if (const LsLightRig* rig = canvas.GetRig(slot)) {
      rigEntries.U32(slot);
      rigEntries.String(rig->name);
      rigEntries.U32(rig->lights);
      rigEntries.U32(rig->occluders);
      rigEntries.Vec3(rig->color);
      rigCount++;
    }
  }
  rigs.U32(rigCount);
  rigs.Raw(rigEntries.bytes.data(), rigEntries.bytes.size());
  file.Chunk("RIGS", rigs);

  LsDocumentWriter brdfs;
  WriteBrdf(brdfs, canvas.GetBrushBrdf());
  const std::vector<LsBrdfParameters>& painted = canvas.GetPaintedBrdfs();
  brdfs.U32(static_cast<uint32_t>(painted.size()));
  for (const LsBrdfParameters& brdf : painted) {
    WriteBrdf(brdfs, brdf);
  }
  file.Chunk("BRDF", brdfs);

  LsDocumentWriter render;
  const LsCanvasRenderSettings& settings = canvas.GetRenderSettings();
  render.F32(settings.exposure);
  render.U32(settings.samplesPerFrame);
  render.U32(settings.maxSamples);
  file.Chunk("REND", render);

  // Written next to the file and moved over it, so a failed save leaves the old file
  std::string temporary = path + ".saving";
  FILE* out = fopen(temporary.c_str(), "wb");
  if (!out) {
    error = "Could not open " + temporary + " for writing.";
    return false;
  }
  bool written = fwrite(file.bytes.data(), 1, file.bytes.size(), out) == file.bytes.size();
  written = fclose(out) == 0 && written;
  if (!written) {
    std::remove(temporary.c_str());
    error = "Could not write " + temporary + ".";
    return false;
  }
  std::remove(path.c_str());
  if (std::rename(temporary.c_str(), path.c_str()) != 0) {
    error = "Could not replace " + path + ".";
    return false;
  }
  return true;
}

// Reads values in the file's layout from a range of bytes. Reading past the end
// gives zeros and marks the reader failed, so a short or broken file is caught
// once at the end instead of after every value.
class LsDocumentReader
{
  const uint8_t* data;
  size_t size;
  size_t offset = 0;
public:
  bool failed = false;

  LsDocumentReader(const uint8_t* data, size_t size) : data(data), size(size) {}

  size_t Remaining() const { return size - offset; }
  bool Raw(void* out, size_t count) {
    if (failed || count > Remaining()) {
      failed = true;
      std::memset(out, 0, count);
      return false;
    }
    std::memcpy(out, data + offset, count);
    offset += count;
    return true;
  }
  const uint8_t* Skip(size_t count) {
    if (failed || count > Remaining()) {
      failed = true;
      return nullptr;
    }
    const uint8_t* start = data + offset;
    offset += count;
    return start;
  }
  uint8_t U8() { uint8_t value; Raw(&value, sizeof(value)); return value; }
  uint32_t U32() { uint32_t value; Raw(&value, sizeof(value)); return value; }
  uint64_t U64() { uint64_t value; Raw(&value, sizeof(value)); return value; }
  float F32() { float value; Raw(&value, sizeof(value)); return value; }
  glm::vec3 Vec3() { float x = F32(), y = F32(), z = F32(); return glm::vec3(x, y, z); }
  glm::vec4 Vec4() { float x = F32(), y = F32(), z = F32(), w = F32(); return glm::vec4(x, y, z, w); }
  std::string String() {
    uint32_t length = U32();
    const uint8_t* bytes = Skip(length);
    return bytes ? std::string(reinterpret_cast<const char*>(bytes), length) : std::string();
  }
};

// Builds the scene from its nodes, which come depth first, parents before children
static bool ReadScene(LsDocumentReader& in, LsScene& scene) {
  uint32_t activeCameraIndex = in.U32();
  uint32_t nodeCount = in.U32();
  std::vector<LsNodeId> ids;
  for (uint32_t index = 0; index < nodeCount && !in.failed; index++) {
    uint32_t parentIndex = in.U32();
    LsSceneNodeProperties properties;
    properties.name = in.String();
    properties.type = static_cast<LsNodeType>(in.U32());
    properties.visible = in.U8() != 0;
    properties.placement.translation = in.Vec3();
    properties.placement.rotation = in.Vec3();
    properties.placement.scale = in.Vec3();
    properties.shape.type = static_cast<LsShapeType>(in.U32());
    properties.shape.params = in.Vec4();
    properties.roles = in.U32();
    properties.lens.verticalFov = in.F32();
    properties.emissionColor = in.Vec3();
    properties.emissionIntensity = in.F32();
    properties.mediumAlbedo = in.Vec3();
    properties.mediumDensity = in.F32();
    if (in.failed || properties.shape.type >= LsShapeType::Count || properties.type > LsNodeType::Camera) {
      return false;
    }

    LsNodeId id = kLsInvalidNode;
    if (index == 0) {
      // The root, which every scene has
      if (parentIndex != UINT32_MAX) {
        return false;
      }
      id = scene.GetRoot();
    }
    else {
      if (parentIndex >= ids.size()) {
        return false;
      }
      LsNodeId parent = ids[parentIndex];
      switch (properties.type) {
        case LsNodeType::Group:  id = scene.AddGroup(properties.name, parent); break;
        case LsNodeType::Shape:  id = scene.AddShape(properties.name, properties.shape, properties.roles, parent); break;
        case LsNodeType::Camera: id = scene.AddCamera(properties.name, properties.lens, parent); break;
      }
    }
    scene.SetProperties(id, properties);
    ids.push_back(id);
  }
  if (activeCameraIndex < ids.size()) {
    scene.SetActiveCamera(ids[activeCameraIndex]);
  }
  return !in.failed;
}

static LsBrdfParameters ReadBrdf(LsDocumentReader& in) {
  LsBrdfParameters brdf;
  brdf.baseColor = in.Vec3();
  brdf.metallic = in.F32();
  brdf.roughness = in.F32();
  brdf.ior = in.F32();
  return brdf;
}

bool LsLoadDocument(const std::string& path, LsDocument& document, std::string& error) {
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    error = "Could not open " + path + ".";
    return false;
  }
  std::vector<uint8_t> bytes;
  std::vector<uint8_t> buffer(1 << 16);
  size_t count;
  while ((count = fread(buffer.data(), 1, buffer.size(), file)) > 0) {
    bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + count);
  }
  fclose(file);

  LsDocumentReader in(bytes.data(), bytes.size());
  char magic[8];
  in.Raw(magic, sizeof(magic));
  if (in.failed || std::memcmp(magic, "LSPAINT", 8) != 0) {
    error = path + " is not a Lustrious Paint document.";
    return false;
  }
  uint32_t version = in.U32();
  if (version > kDocumentVersion) {
    error = path + " was saved by a newer version of Lustrious Paint.";
    return false;
  }

  bool sawScene = false, sawCanvas = false;
  while (in.Remaining() > 0 && !in.failed) {
    char tag[4];
    in.Raw(tag, sizeof(tag));
    uint64_t size = in.U64();
    const uint8_t* data = in.Skip(static_cast<size_t>(size));
    if (!data) {
      break;
    }
    LsDocumentReader chunk(data, static_cast<size_t>(size));
    std::string name(tag, 4);
    if (name == "SCEN") {
      sawScene = ReadScene(chunk, document.scene);
      if (!sawScene) {
        error = "The scene in " + path + " is damaged.";
        return false;
      }
    }
    else if (name == "CANV") {
      LsCanvasAttributes& attributes = document.attributes;
      attributes.width = chunk.U32();
      attributes.height = chunk.U32();
      size_t pixels = size_t(attributes.width)*attributes.height;
      if (chunk.failed || chunk.Remaining() < pixels*sizeof(uint32_t)*5) {
        error = "The canvas in " + path + " is damaged.";
        return false;
      }
      auto read = [&](auto& target) {
        target.resize(pixels);
        chunk.Raw(target.data(), pixels*sizeof(uint32_t));
      };
      read(attributes.distance);
      read(attributes.normal);
      read(attributes.materialColor);
      read(attributes.materialSurface);
      read(attributes.rig);
      sawCanvas = true;
    }
    else if (name == "LITE") {
      uint32_t lightCount = chunk.U32();
      for (uint32_t i = 0; i < lightCount && !chunk.failed; i++) {
        uint32_t slot = chunk.U32();
        LsLocalLight light;
        light.name = chunk.String();
        light.direction = chunk.Vec3();
        light.color = chunk.Vec3();
        light.intensity = chunk.F32();
        light.size = chunk.F32();
        light.shadows = chunk.U8() != 0;
        if (slot < kLsMaxLocalLights) {
          document.lights.push_back({ slot, light });
        }
      }
    }
    else if (name == "OCCL") {
      uint32_t occluderCount = chunk.U32();
      for (uint32_t i = 0; i < occluderCount && !chunk.failed; i++) {
        uint32_t slot = chunk.U32();
        LsOccluder occluder;
        occluder.name = chunk.String();
        occluder.direction = chunk.Vec3();
        occluder.size = chunk.F32();
        occluder.softness = chunk.F32();
        occluder.strength = chunk.F32();
        if (slot < kLsMaxOccluders) {
          document.occluders.push_back({ slot, occluder });
        }
      }
    }
    else if (name == "RIGS") {
      uint32_t rigCount = chunk.U32();
      for (uint32_t i = 0; i < rigCount && !chunk.failed; i++) {
        uint32_t slot = chunk.U32();
        LsLightRig rig;
        rig.name = chunk.String();
        rig.lights = chunk.U32();
        rig.occluders = chunk.U32();
        rig.color = chunk.Vec3();
        if (slot > 0 && slot < kLsMaxRigs) {
          document.rigs.push_back({ slot, rig });
        }
      }
    }
    else if (name == "BRDF") {
      document.brushBrdf = ReadBrdf(chunk);
      uint32_t paintedCount = chunk.U32();
      for (uint32_t i = 0; i < paintedCount && !chunk.failed; i++) {
        document.paintedBrdfs.push_back(ReadBrdf(chunk));
      }
    }
    else if (name == "REND") {
      document.renderSettings.exposure = chunk.F32();
      document.renderSettings.samplesPerFrame = chunk.U32();
      document.renderSettings.maxSamples = chunk.U32();
    }
    if (chunk.failed) {
      error = "The " + name + " section of " + path + " is damaged.";
      return false;
    }
  }
  if (in.failed || !sawScene || !sawCanvas) {
    error = path + " is incomplete.";
    return false;
  }
  return true;
}
