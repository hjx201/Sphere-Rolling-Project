/***************************
 * File: vshader42.glsl:
 *   A simple vertex shader.
 *
 * - Vertex attributes (positions & colors) for all vertices are sent
 *   to the GPU via a vertex buffer object created in the OpenGL program.
 *
 * - This vertex shader uses the Model-View and Projection matrices passed
 *   on from the OpenGL program as uniform variables of type mat4.
 ***************************/

// #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

in  vec3 pVelocity;
in  vec3 pColor;
out vec4 color;

out float y;

uniform float time;

uniform mat4 ModelView;
uniform mat4 Projection;

void main() 
{
  vec3 initPos = vec3(0.0, 0.1, 0.0);
  vec4 vColor4 = vec4(pColor.r, pColor.g, pColor.b, 1.0); 
  float a = -0.00000049;
  
  float x = 0.001 * pVelocity.x * time;
  float z = 0.001 * pVelocity.z * time;
  y = 0.1 + (0.001 * pVelocity.y * time) + 0.5 * time * time * a;
  
  vec4 pPosition = vec4(x, y, z, 1.0);
  
  gl_Position = Projection * ModelView * pPosition;

  color = vColor4;
} 
