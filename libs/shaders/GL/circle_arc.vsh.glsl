layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_pxOffset;
layout (location = 2) in vec2 a_colorTexCoords;
layout (location = 3) in float a_from;
layout (location = 4) in float a_to;
layout (location = 5) in float a_smallRadius;

#ifdef ENABLE_VTF
layout (location = 0) out LOW_P vec4 v_color;
#else
layout (location = 1) out vec2 v_colorTexCoords;
#endif
layout (location = 2) out vec2 v_uv;
layout (location = 3) out float v_from;
layout (location = 4) out float v_to;
layout (location = 5) out float v_smallRadius;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
};

#ifdef ENABLE_VTF
layout (binding = 1) uniform sampler2D u_colorTex;
#endif

void main()
{
  vec4 p = vec4(a_position, 1) * u_modelView;
  vec2 poffset = (vec4(a_position.xy + a_pxOffset, 0.0, 1) * u_modelView).xy;
  vec2 off = length(a_pxOffset) * normalize(poffset - p.xy);
  vec4 pos = vec4(a_pxOffset, 0, 0) + p;
  gl_Position = applyPivotTransform(pos * u_projection, u_pivotTransform, 0.0);
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
  v_from = a_from;
  v_to = a_to;
  v_smallRadius = a_smallRadius;
  v_uv = 2.0 * normalize(off); // `2 *` to have incircle with radius 1
}
