/* 
File Name: "fshader53.glsl":
           Fragment Shader
*/

// #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

in  vec4 color;
out vec4 fColor;

in float fogflag;
in float fogdist;

uniform sampler2D texture_2D;
uniform sampler1D texture_1D;

in  vec2 TexCoord;
in float isTexturedf;

//from sphere texture calculations
in float TexTypef;
in vec2 SphereTexCoords;

in vec2 SphereLatCoords;
in float isLatticedf;

vec4 mixFog(vec4 color);

void main() 
{ 
  if(isLatticedf == 1.0){
    if(fract(4*SphereLatCoords.x) < 0.35 
    && fract(4*SphereLatCoords.y) < 0.35
    && fract(4*SphereLatCoords.x) > 0 
    && fract(4*SphereLatCoords.y) > 0    ) discard;
  }

  if(isTexturedf == 1.0){
    fColor = color * texture( texture_2D, TexCoord );
  }
  else{

    
    if (TexTypef == 1.0)//checker texture{
    {
      if(texture( texture_2D, SphereTexCoords)[0] == 0){
        fColor = color * vec4(0.9, 0.1, 0.1, 1.0); //switch to red if green
      }
      else {
      fColor = color * texture( texture_2D, SphereTexCoords);
      }
    }
    else if (TexTypef == 2.0) //stripes
    {
      fColor = color * texture( texture_1D, SphereTexCoords.x);
    }
    else{
      fColor = color;
    }
  }
  fColor = mixFog(fColor);
} 

vec4 mixFog(vec4 color){
  vec4 newcolor = color;
  
  vec4 fogcolor = vec4(0.7, 0.7, 0.7, 0.5);
  
  float foga;
  
  if(fogflag == 0.0){
    return newcolor;
  }
  else if(fogflag == 1.0) //linear
  {
    float distclamped = clamp(fogdist, 0.0, 18.0);
    foga = (18.0-distclamped)/18.0;
    
    //newcolor = mix(color, fogcolor, foga);
  }
  else if(fogflag == 2.0) //exp
  {
    foga = exp(-0.09 * fogdist);
    //newcolor = mix(color, fogcolor, 1-foga);
  }
  else if(fogflag == 3.0) //expsquare
  {
    foga = exp(-0.09 * 0.09 * fogdist * fogdist);  
    //newcolor = mix(color, fogcolor, 1-foga);
  }

    newcolor = mix(fogcolor, color, foga);
    return newcolor;
}