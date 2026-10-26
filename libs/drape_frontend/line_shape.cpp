#include "drape_frontend/line_shape.hpp"

#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/binding_info.hpp"
#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"
#include "drape/graphics_context.hpp"
#include "drape/render_state.hpp"
#include "drape/support_manager.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "indexer/scales.hpp"

#include "geometry/rect2d.hpp"

#include "coding/point_coding.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace df
{
namespace
{
class TextureCoordGenerator
{
public:
  explicit TextureCoordGenerator(dp::TextureManager::StippleRegion const & region)
    : m_region(region)
    , m_maskSize(m_region.GetMaskPixelSize())
  {}

  glsl::vec4 GetTexCoordsByDistance(float distance, bool isLeft) const
  {
    m2::RectF const & texRect = m_region.GetTexRect();
    return {distance / GetMaskLength(), texRect.minX(), texRect.SizeX(), isLeft ? texRect.minY() : texRect.maxY()};
  }

  uint32_t GetMaskLength() const { return m_maskSize.x; }

  dp::TextureManager::StippleRegion const & GetRegion() const { return m_region; }

private:
  dp::TextureManager::StippleRegion const m_region;
  m2::PointU const m_maskSize;
};

struct BaseBuilderParams
{
  dp::TextureManager::ColorRegion m_color;
  float m_pxHalfWidth;
  float m_pxOffset;
  float m_depth;
  bool m_depthTestEnabled;
  DepthLayer m_depthLayer;
  dp::LineCap m_cap;
  dp::LineJoin m_join;
};

template <typename TVertex>
class BaseLineBuilder : public LineShapeInfo
{
public:
  BaseLineBuilder(BaseBuilderParams const & params, size_t geomsSize, size_t joinsSize)
    : m_params(params)
    , m_colorCoord(glsl::ToVec2(params.m_color.GetTexRect().Center()))
  {
    m_geometry.reserve(geomsSize);
    // m_joinGeom.reserve(joinsSize);
  }

  dp::BindingInfo const & GetBindingInfo() override { return TVertex::GetBindingInfo(); }

  ref_ptr<void> GetLineData() override { return make_ref(m_geometry.data()); }

  uint32_t GetLineSize() override { return static_cast<uint32_t>(m_geometry.size()); }

  //  ref_ptr<void> GetJoinData() override
  //  {
  //    return make_ref(m_joinGeom.data());
  //  }

  //  uint32_t GetJoinSize() override
  //  {
  //    return static_cast<uint32_t>(m_joinGeom.size());
  //  }

  float GetHalfWidth() { return m_params.m_pxHalfWidth; }

  float GetOffset() { return m_params.m_pxOffset; }

  dp::BindingInfo const & GetJoinBindingInfo() override { return GetBindingInfo(); }

  dp::RenderState GetJoinState() override { return GetState(); }

  ref_ptr<void> GetJoinData() override { return ref_ptr<void>(); }

  uint32_t GetJoinSize() override { return 0; }

  dp::BindingInfo const & GetCapBindingInfo() override { return GetBindingInfo(); }

  dp::RenderState GetCapState() override { return GetState(); }

  ref_ptr<void> GetCapData() override { return ref_ptr<void>(); }

  uint32_t GetCapSize() override { return 0; }

  float GetSide(bool isLeft) const { return isLeft ? 1.0f : -1.0f; }

protected:
  using V = TVertex;
  using TGeometryBuffer = gpu::VBReservedSizeT<V>;

  TGeometryBuffer m_geometry;
  // TGeometryBuffer m_joinGeom;

  BaseBuilderParams m_params;
  glsl::vec2 const m_colorCoord;
};

class SolidLineBuilder : public BaseLineBuilder<gpu::LineVertex>
{
  using TBase = BaseLineBuilder<gpu::LineVertex>;
  using TPxOffset = gpu::LineVertex::TPxOffset;

  struct JoinVertex
  {
    using TPosition = gpu::LineVertex::TPosition;
    using TPxOffset = gpu::LineVertex::TPxOffset;
    using TTexCoord = gpu::LineVertex::TTexCoord;
    using TAngle = float;

    JoinVertex() {}
    JoinVertex(TPosition const & pos, TPxOffset const & pxOffset, TTexCoord const & color, TAngle const & from,
               TAngle const & to, float const & smallRadius)
      : m_position(pos)
      , m_pxOffset(pxOffset)
      , m_color(color)
      , m_from(from)
      , m_to(to)
      , m_smallRadius(smallRadius)
    {}

    TPosition m_position;
    TPxOffset m_pxOffset;
    TTexCoord m_color;
    TAngle m_from;
    TAngle m_to;
    float m_smallRadius;
  };

  struct CapVertex
  {
    using TPosition = gpu::LineVertex::TPosition;
    using TPxOffset = gpu::LineVertex::TPxOffset;
    using TTexCoord = gpu::LineVertex::TTexCoord;

    CapVertex() {}
    CapVertex(TPosition const & pos, TPxOffset const & pxOffset, TTexCoord const & color)
      : m_position(pos)
      , m_pxOffset(pxOffset)
      , m_color(color)
    {}

    TPosition m_position;
    TPxOffset m_pxOffset;
    TTexCoord m_color;
  };

  using TCapBuffer = gpu::VBUnknownSizeT<CapVertex>;
  using TJoinBuffer = gpu::VBUnknownSizeT<JoinVertex>;

public:
  using BuilderParams = BaseBuilderParams;

  SolidLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : TBase(params, pointsInSpline * 2, (pointsInSpline - 2) * 8)
  {}

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::Line, m_params.m_depthLayer);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    return state;
  }

  dp::BindingInfo const & GetJoinBindingInfo() override
  {
    ASSERT(!m_joinGeometry.empty(), ());

    static std::unique_ptr<dp::BindingInfo> s_joinInfo;
    if (s_joinInfo == nullptr)
    {
      dp::BindingFiller<JoinVertex> filler(6);
      filler.FillDecl<JoinVertex::TPosition>("a_position");
      filler.FillDecl<JoinVertex::TPxOffset>("a_pxOffset");
      filler.FillDecl<JoinVertex::TTexCoord>("a_colorTexCoords");
      filler.FillDecl<JoinVertex::TAngle>("a_from");
      filler.FillDecl<JoinVertex::TAngle>("a_to");
      filler.FillDecl<float>("a_smallRadius");

      s_joinInfo.reset(new dp::BindingInfo(filler.m_info));
    }

    return *s_joinInfo;
  }

  dp::BindingInfo const & GetCapBindingInfo() override
  {
    ASSERT(!m_capGeometry.empty(), ());

    static std::unique_ptr<dp::BindingInfo> s_capInfo;
    if (s_capInfo == nullptr)
    {
      dp::BindingFiller<CapVertex> filler(3);
      filler.FillDecl<CapVertex::TPosition>("a_position");
      filler.FillDecl<CapVertex::TPxOffset>("a_pxOffset");
      filler.FillDecl<CapVertex::TTexCoord>("a_colorTexCoords");

      s_capInfo.reset(new dp::BindingInfo(filler.m_info));
    }

    return *s_capInfo;
  }

  dp::RenderState GetJoinState() override
  {
    ASSERT(!m_joinGeometry.empty(), ());

    auto state = CreateRenderState(gpu::Program::LineJoin, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDepthFunction(dp::TestFunction::Less);
    return state;
  }

  dp::RenderState GetCapState() override
  {
    ASSERT(!m_capGeometry.empty(), ());

    auto state = CreateRenderState(gpu::Program::LineCap, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDepthFunction(dp::TestFunction::Less);
    return state;
  }

  ref_ptr<void> GetJoinData() override { return make_ref<void>(m_joinGeometry.data()); }

  uint32_t GetJoinSize() override { return static_cast<uint32_t>(m_joinGeometry.size()); }

  ref_ptr<void> GetCapData() override { return make_ref<void>(m_capGeometry.data()); }

  uint32_t GetCapSize() override { return static_cast<uint32_t>(m_capGeometry.size()); }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & pxOffset)
  {
    m_geometry.emplace_back(pivot, TPxOffset(pxOffset), m_colorCoord);
  }

  void SubmitJoin(glsl::vec2 const & pos, glsl::vec2 const & from, glsl::vec2 const & to, bool const & onRight)
  {
    if (m_params.m_join == dp::RoundJoin)
      CreateArcJoin(pos, from, to, onRight);
  }

  void SubmitCap(glsl::vec2 const & pos)
  {
    if (m_params.m_cap != dp::ButtCap)
      CreateRoundCap(pos);
  }

private:
  void CreateRoundCap(glsl::vec2 const & pos)
  {
    // Here we use an equilateral triangle to render circle (incircle of a triangle).
    static float constexpr kSqrt3 = 1.732050808f;
    float const radius = GetHalfWidth();

    m_capGeometry.emplace_back(CapVertex::TPosition(pos, m_params.m_depth),
                               CapVertex::TPxOffset(-radius * kSqrt3, -radius), CapVertex::TTexCoord(m_colorCoord));
    m_capGeometry.emplace_back(CapVertex::TPosition(pos, m_params.m_depth),
                               CapVertex::TPxOffset(radius * kSqrt3, -radius), CapVertex::TTexCoord(m_colorCoord));
    m_capGeometry.emplace_back(CapVertex::TPosition(pos, m_params.m_depth), CapVertex::TPxOffset(0, 2.0f * radius),
                               CapVertex::TTexCoord(m_colorCoord));
  }

  void CreateArcJoin(glsl::vec2 const & pos, glsl::vec2 const & from, glsl::vec2 const & to, bool const & onRight)
  {
    // Here we use an equilateral triangle to render circle arc (part of incircle of a triangle).
    static float constexpr kSqrt3 = 1.732050808f;

    float radius;
    float smallRadius;
    if (onRight)
    {
      radius = GetOffset() + GetHalfWidth();
      smallRadius = GetOffset() - GetHalfWidth();
    }
    else
    {
      radius = -(GetOffset() - GetHalfWidth());
      smallRadius = -(GetOffset() + GetHalfWidth());
    }

    if (radius <= 0)
      return;

    float normalizedSmallRadius = (smallRadius < 0 ? 0 : smallRadius) / radius;

    m_joinGeometry.emplace_back(JoinVertex::TPosition(pos, m_params.m_depth),
                                JoinVertex::TPxOffset(-radius * kSqrt3, -radius), JoinVertex::TTexCoord(m_colorCoord),
                                std::atan2(from.y, from.x), std::atan2(to.y, to.x), normalizedSmallRadius);
    m_joinGeometry.emplace_back(JoinVertex::TPosition(pos, m_params.m_depth),
                                JoinVertex::TPxOffset(radius * kSqrt3, -radius), JoinVertex::TTexCoord(m_colorCoord),
                                std::atan2(from.y, from.x), std::atan2(to.y, to.x), normalizedSmallRadius);
    m_joinGeometry.emplace_back(JoinVertex::TPosition(pos, m_params.m_depth), JoinVertex::TPxOffset(0, 2.0f * radius),
                                JoinVertex::TTexCoord(m_colorCoord), std::atan2(from.y, from.x), std::atan2(to.y, to.x),
                                normalizedSmallRadius);
  }

private:
  TCapBuffer m_capGeometry;
  TJoinBuffer m_joinGeometry;
};

class SimpleSolidLineBuilder : public BaseLineBuilder<gpu::AreaVertex>
{
  using TBase = BaseLineBuilder<gpu::AreaVertex>;

public:
  using BuilderParams = BaseBuilderParams;

  SimpleSolidLineBuilder(BuilderParams const & params, size_t pointsInSpline, int lineWidth)
    : TBase(params, pointsInSpline, 0)
    , m_lineWidth(lineWidth)
  {}

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::AreaOutline, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDrawAsLine(true);
    state.SetLineWidth(m_lineWidth);
    return state;
  }

  void SubmitVertex(glsl::vec3 const & pivot) { m_geometry.emplace_back(pivot, m_colorCoord); }

private:
  int m_lineWidth;
};

class DashedLineBuilder : public BaseLineBuilder<gpu::DashedLineVertex>
{
  using TBase = BaseLineBuilder<gpu::DashedLineVertex>;
  using TNormal = gpu::LineVertex::TNormal;

public:
  struct BuilderParams : BaseBuilderParams
  {
    dp::TextureManager::StippleRegion m_stipple;
    float m_baseGtoP;
  };

  DashedLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : TBase(params, pointsInSpline * 8, (pointsInSpline - 2) * 8)
    , m_texCoordGen(params.m_stipple)
    , m_baseGtoPScale(params.m_baseGtoP)
  {}

  float GetMaskLengthG() const { return m_texCoordGen.GetMaskLength() / m_baseGtoPScale; }

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::DashedLine, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetMaskTexture(m_texCoordGen.GetRegion().GetTexture());
    return state;
  }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft, float offsetFromStart)
  {
    float const halfWidth = GetHalfWidth();
    m_geometry.emplace_back(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)), m_colorCoord,
                            m_texCoordGen.GetTexCoordsByDistance(offsetFromStart, isLeft));
  }

private:
  TextureCoordGenerator m_texCoordGen;
  float const m_baseGtoPScale;
};
}  // namespace

LineShape::LineShape(m2::SharedSpline const & spline, LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
  , m_isSimple(false)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

template <typename TBuilder>
void LineShape::Construct(TBuilder & builder) const
{
  ASSERT(false, ("No implementation"));
}

template <class FnT>
void LineShape::ForEachSplineSection(FnT && fn) const
{
  std::vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT(!path.empty(), ());
  size_t const sz = path.size() - 1;

  m2::PointD lastTangent = {0, 0};

  for (size_t i = 0, j = 1; j <= sz; ++j)
  {
    /// @todo Make this kind of filtration in Spline?
    if (path[i].EqualDxDy(path[j], kMwmPointAccuracy) && j < sz)
      continue;

    std::pair<m2::PointD, double> tanlen;
    if (j == i + 1)
    {
      // Fast path - take calculated tangent and length.
      tanlen = m_spline->GetTangentAndLength(i);
    }
    else
    {
      tanlen.first = path[j] - path[i];
      tanlen.second = tanlen.first.Length();
      tanlen.first = tanlen.first / tanlen.second;
    }

    m2::PointD fromCap = lastTangent.Ort();
    if (m2::DotProduct(fromCap, tanlen.first) > 0)
      fromCap *= -1;

    m2::PointD toCap = tanlen.first.Ort();
    if (m2::DotProduct(toCap, lastTangent) < 0)
      toCap *= -1;

    if (m2::CrossProduct(fromCap, toCap) < 0)
      std::swap(fromCap, toCap);

    glsl::vec2 const tangent = glsl::ToVec2(tanlen.first);

    bool capOnRight = (m2::CrossProduct(lastTangent, tanlen.first) > 0);

    // p1, p2, tangent, tangent length, left normal, right normal, from cap, to cap, flag
    fn(ToShapeVertex2(path[i]), ToShapeVertex2(path[j]), tangent, tanlen.second, {-tangent.y, tangent.x},
       {tangent.y, -tangent.x}, glsl::ToVec2(fromCap), glsl::ToVec2(toCap),
       (i == 0 ? 0x1 : 0) + (j == sz ? 0x2 : 0) + (capOnRight ? 0x4 : 0));

    lastTangent = tanlen.first;

    i = j;
  }
}

// Specialization optimized for dashed lines.
template <>
void LineShape::Construct<DashedLineBuilder>(DashedLineBuilder & builder) const
{
  float constexpr toShapeFactor = kShapeCoordScalar;  // the same as in ToShapeVertex2

  // Each segment should lie in pattern mask according to the "longest" possible pixel length in current tile.
  // Since, we calculate vertices once, usually for the "smallest" tile scale, need to apply divide factor here.
  // In other words, if m_baseGtoPScale = Scale(tileLevel), we should use Scale(tileLevel + 1) to calculate
  // 'maskLengthG'.
  /// @todo Logically, the factor should be 2, but drawing artifacts are still present at higher visual scales.
  /// Use 3 for the best quality, but need to review here, probably I missed something.
  float const maskLengthG = builder.GetMaskLengthG() / 3;

  float offset = 0;
  ForEachSplineSection([&](glsl::vec2 const & p1, glsl::vec2 const & p2, glsl::vec2 const & tangent, float toDraw,
                           glsl::vec2 const & leftNormal, glsl::vec2 const & rightNormal, glsl::vec2, glsl::vec2, int)
  {
    glsl::vec2 currPivot = p1;
    do
    {
      glsl::vec2 nextPivot;
      float nextOffset = offset + toDraw;
      if (maskLengthG >= nextOffset)
      {
        // Fast lane, where most of segments, that fit into mask, should draw.
        nextPivot = p2;
        toDraw = 0;
      }
      else
      {
        // Break path section here.
        float const len = maskLengthG - offset;
        ASSERT_GREATER(len, 0, ());
        nextPivot = currPivot + tangent * (len * toShapeFactor);

        nextOffset = maskLengthG;
        toDraw -= len;
      }

      builder.SubmitVertex({currPivot, m_params.m_depth}, rightNormal, false /* isLeft */, offset);
      builder.SubmitVertex({currPivot, m_params.m_depth}, leftNormal, true /* isLeft */, offset);
      builder.SubmitVertex({nextPivot, m_params.m_depth}, rightNormal, false /* isLeft */, nextOffset);
      builder.SubmitVertex({nextPivot, m_params.m_depth}, leftNormal, true /* isLeft */, nextOffset);

      currPivot = nextPivot;
      offset = nextOffset;
      if (offset >= maskLengthG)
        offset = 0;
    }
    while (toDraw > 0);
  });
}

// Specialization optimized for solid lines.
template <>
void LineShape::Construct<SolidLineBuilder>(SolidLineBuilder & builder) const
{
  // Skip joins generation for thin lines.
  bool const generateJoins = builder.GetHalfWidth() + std::abs(builder.GetOffset()) > 2.5f;

  float halfWidth = builder.GetHalfWidth();
  float pxOffset = builder.GetOffset();
  ForEachSplineSection([&](glsl::vec2 const & p1, glsl::vec2 const & p2, glsl::vec2 const & tangent, double,
                           glsl::vec2 const & leftNormal, glsl::vec2 const & rightNormal, glsl::vec2 const & capFrom,
                           glsl::vec2 const & capTo, int flag)
  {
    builder.SubmitVertex({p1, m_params.m_depth}, (pxOffset + halfWidth) * rightNormal);
    builder.SubmitVertex({p1, m_params.m_depth}, (pxOffset - halfWidth) * rightNormal);
    builder.SubmitVertex({p2, m_params.m_depth}, (pxOffset + halfWidth) * rightNormal);
    builder.SubmitVertex({p2, m_params.m_depth}, (pxOffset - halfWidth) * rightNormal);

    // Generate joins.
    if (flag & 0x1)  // p1 - first point
      builder.SubmitCap(p1);
    else if (generateJoins)  // p1 - middle point
      builder.SubmitJoin(p1, capFrom, capTo, (flag & 0x4));
    if (flag & 0x2)  // p2 - last point
      builder.SubmitCap(p2);
  });
}

// Specialization optimized for simple solid lines.
template <>
void LineShape::Construct<SimpleSolidLineBuilder>(SimpleSolidLineBuilder & builder) const
{
  std::vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT_GREATER(path.size(), 1, ());

  // Build geometry.
  for (m2::PointD const & pt : path)
    builder.SubmitVertex(glsl::vec3(ToShapeVertex2(pt), m_params.m_depth));
}

bool LineShape::CanBeSimplified(int & lineWidth) const
{
  // Disable simplification for world map.
  if (m_params.m_zoomLevel > 0 && m_params.m_zoomLevel <= scales::GetUpperCountryScale())
    return false;

  float const width = std::min(2.5f, dp::SupportManager::Instance().GetMaxLineWidth());
  if (m_params.m_width <= width)
  {
    lineWidth = std::max(1, static_cast<int>(m_params.m_width));
    return true;
  }

  lineWidth = 1;
  return false;
}

void LineShape::Prepare(ref_ptr<dp::TextureManager> textures) const
{
  auto commonParamsBuilder = [this, textures](BaseBuilderParams & p)
  {
    dp::TextureManager::ColorRegion colorRegion;
    textures->GetColorRegion(m_params.m_color, colorRegion);

    p.m_cap = m_params.m_cap;
    p.m_color = colorRegion;
    p.m_pxOffset = m_params.m_pxOffset;
    p.m_depthTestEnabled = m_params.m_depthTestEnabled;
    p.m_depth = m_params.m_depth;
    p.m_depthLayer = m_params.m_depthLayer;
    p.m_join = m_params.m_join;
    p.m_pxHalfWidth = m_params.m_width / 2;
  };

  if (m_params.m_pattern.empty())
  {
    int lineWidth = 1;
    m_isSimple = CanBeSimplified(lineWidth);
    if (m_isSimple)
    {
      SimpleSolidLineBuilder::BuilderParams p;
      commonParamsBuilder(p);

      auto builder = std::make_unique<SimpleSolidLineBuilder>(p, m_spline->GetPath().size(), lineWidth);
      Construct<SimpleSolidLineBuilder>(*builder);
      m_lineShapeInfo = std::move(builder);
    }
    else
    {
      SolidLineBuilder::BuilderParams p;
      commonParamsBuilder(p);

      auto builder = std::make_unique<SolidLineBuilder>(p, m_spline->GetPath().size());
      Construct<SolidLineBuilder>(*builder);
      m_lineShapeInfo = std::move(builder);
    }
  }
  else
  {
    dp::TextureManager::StippleRegion maskRegion;
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

    DashedLineBuilder::BuilderParams p;
    commonParamsBuilder(p);
    p.m_stipple = maskRegion;
    p.m_baseGtoP = static_cast<float>(m_params.m_baseGtoPScale);

    auto builder = std::make_unique<DashedLineBuilder>(p, m_spline->GetPath().size());
    Construct<DashedLineBuilder>(*builder);
    m_lineShapeInfo = std::move(builder);
  }
}

void LineShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                     ref_ptr<dp::TextureManager> textures) const
{
  if (!m_lineShapeInfo)
    Prepare(textures);

  ASSERT(m_lineShapeInfo != nullptr, ());
  dp::RenderState state = m_lineShapeInfo->GetState();
  dp::AttributeProvider provider(1, m_lineShapeInfo->GetLineSize());
  provider.InitStream(0, m_lineShapeInfo->GetBindingInfo(), m_lineShapeInfo->GetLineData());
  if (!m_isSimple)
  {
    batcher->InsertListOfStrip(context, state, make_ref(&provider), dp::Batcher::VertexPerQuad);

    // Not used, keep comment for possible usage. LineJoin::RoundJoin is processed as _Cap_.
    //    uint32_t const joinSize = m_lineShapeInfo->GetJoinSize();
    //    if (joinSize > 0)
    //    {
    //      dp::AttributeProvider joinsProvider(1, joinSize);
    //      joinsProvider.InitStream(0, m_lineShapeInfo->GetBindingInfo(), m_lineShapeInfo->GetJoinData());
    //      batcher->InsertTriangleList(context, state, make_ref(&joinsProvider));
    //    }

    uint32_t const capSize = m_lineShapeInfo->GetCapSize();
    if (capSize > 0)
    {
      dp::AttributeProvider capProvider(1, capSize);
      capProvider.InitStream(0, m_lineShapeInfo->GetCapBindingInfo(), m_lineShapeInfo->GetCapData());
      batcher->InsertTriangleList(context, m_lineShapeInfo->GetCapState(), make_ref(&capProvider));
    }

    uint32_t const joinSize = m_lineShapeInfo->GetJoinSize();
    if (joinSize > 0)
    {
      dp::AttributeProvider joinProvider(1, joinSize);
      joinProvider.InitStream(0, m_lineShapeInfo->GetJoinBindingInfo(), m_lineShapeInfo->GetJoinData());
      batcher->InsertTriangleList(context, m_lineShapeInfo->GetJoinState(), make_ref(&joinProvider));
    }
  }
  else
  {
    batcher->InsertLineStrip(context, state, make_ref(&provider));
  }
}
}  // namespace df
