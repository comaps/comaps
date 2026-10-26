layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_pxOffset;
layout (location = 2) in vec2 a_colorTexCoord;

#ifdef ENABLE_VTF
layout (location = 0) out LOW_P vec4 v_color;
#else
layout (location = 1) out vec2 v_colorTexCoord;
#endif

//layout (location = 2) out vec2 v_halfLength;

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
  float pxOffset = length(a_pxOffset);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  if (pxOffset != 0.0)
  {
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + a_pxOffset,
                                                    u_modelView, pxOffset);
  }
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoord);
#else
  v_colorTexCoord = a_colorTexCoord;
#endif
  //v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z));
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
