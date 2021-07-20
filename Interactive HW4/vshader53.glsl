/* 
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/

// #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version


//passed in from outside shader
in  vec4 vPosition;
in  vec3 vNormal;

in  vec3 vPosition3;

in  vec3 vColor3;
in  vec4 vColor4;

in vec2 vTexCoord;
out vec2 TexCoord;

out vec4 color;
out float fogflag;
out float fogdist;

uniform vec4 MaterialAmbient, MaterialDiffuse, MaterialSpecular;

uniform vec4 LightAmbient, LightDiffuse, LightSpecular;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat3 Normal_Matrix;
uniform vec4 LightPosition;   // Must be in Eye Frame
uniform float Shininess;

uniform float ConstAtt;  // Constant Attenuation
uniform float LinearAtt; // Linear Attenuation
uniform float QuadAtt;   // Quadratic Attenuation

//directional stuff here
uniform vec4 DirAmbient;
uniform vec4 DirSpecular;
uniform vec4 DirDiffuse;
uniform vec3 DirLightDir;

//global ambient light
uniform vec4 GlobAmbient;

uniform float EnablePosLight;
uniform float isPointSource;
uniform float PerformShading;

uniform float hasAlpha;

uniform float isTextured = 0.0;  //specifically, flag for textured floor
out float isTexturedf;

vec4 calcDirectionalColor();
vec4 calcPositionalColor();

uniform vec3 SpotDirection;
uniform float SpotAngle;
uniform float SpotExp;

uniform float FogType;

uniform vec4 Eye;

//boolean nightmare cont.
uniform float isTexSphere;
uniform float isTexSlanted;
uniform float isTexEye;
uniform float TexType;  //sphere's texture type
out float TexTypef;

uniform float isLatticed;
uniform float isLatTilted;
out float isLatticedf;

void setFog();
vec2 calcTexCoords(float isSlant, vec3 posi);
vec2 calcTexCoords1D(float isSlant, vec3 posi);

vec2 calcLatCoords(float isSlant, vec3 posi);

out vec2 SphereTexCoords;
out vec2 SphereLatCoords;

vec4 PosLat; //this variable is literally here only so that i can reuse the lattice method for shadows lol

void main()
{
  setFog();
  
  if(PerformShading == 1.0){
    PosLat = vPosition;
    
    gl_Position = Projection * ModelView * vPosition;
    
    color = GlobAmbient * MaterialAmbient + calcDirectionalColor() + calcPositionalColor();
  }
  else{
    vec4 vPosition4 = vec4(vPosition3.x, vPosition3.y, vPosition3.z, 1.0);
    PosLat = vPosition4;
    
    gl_Position = Projection * ModelView * vPosition4;
    
    if (hasAlpha == 1.0){
      color = vColor4;
    }
    else if (hasAlpha == 0.0){  //i have to do this for blending shadows so might as well streamline it
      color = vec4(vColor3.r, vColor3.g, vColor3.b, 1.0); 
    }
  }
  
  //for floor
  if (isTextured == 1.0){
    TexCoord = vTexCoord;
  }
  isTexturedf = isTextured;
  

  //for sphere
  if(isTexSphere == 1.0){
    vec3 posi;
    if (isTexEye == 0.0){
      posi = vPosition.xyz;
    }
    else{
      posi = (ModelView * vPosition).xyz;
    }
    
    if (TexType == 1.0){
      SphereTexCoords = calcTexCoords(isTexSlanted, posi);
    }
    else if (TexType == 2.0){
      SphereTexCoords = calcTexCoords1D(isTexSlanted, posi);
    }
    TexTypef = TexType;
  }
  else{
    TexTypef = 0.0;
  }
  
  if(isLatticed == 1.0){
    SphereLatCoords = calcLatCoords(isLatTilted, PosLat.xyz);
  }
  isLatticedf = isLatticed;

}

void setFog(){
  fogflag = FogType;

  vec3 pos = (ModelView * vPosition).xyz;
  //fogdist = length(pos - Eye.xyz);
  
  fogdist = length(pos - (ModelView*Eye).xyz);
}

//for 1d
vec2 calcTexCoords1D(float isSlant, vec3 posi){
  float s;
  if(isSlant == 0){
    s = 2.5 * posi.x;
  }
  else{
    s = 1.5 * (posi.x + posi.y + posi.z);
  }
  return vec2(s, 0.0);
}

//usable for 2d, lattice
vec2 calcTexCoords(float isSlant, vec3 posi){
  float s, t;
  if(isSlant == 0){
    s = 0.75 * (posi.x + 1);
    t = 0.75 * (posi.y + 1);
  }
  else{
    s = 0.45 * (posi.x + posi.y + posi.z);
    t = 0.45 * (posi.x - posi.y + posi.z);
  }
  return vec2(s, t);
}

vec2 calcLatCoords(float isSlant, vec3 posi){
  float s, t;
  if(isSlant == 0){
    s = 0.5 * (posi.x + 1);
    t = 0.5 * (posi.y + 1);
  }
  else{
    s = 0.3 * (posi.x + posi.y + posi.z);
    t = 0.3 * (posi.x - posi.y + posi.z);
  }
  return vec2(s, t);
}

vec4 calcDirectionalColor(){
  vec3 pos = (ModelView * vPosition).xyz;

  vec4 AmbientProduct = DirAmbient * MaterialAmbient;
  vec4 DiffuseProduct = DirDiffuse * MaterialDiffuse;
  vec4 SpecularProduct = DirSpecular * MaterialSpecular;

  vec3 L = normalize(-DirLightDir);
  vec3 E = normalize(-pos);
  vec3 H = normalize ( L + E );

  vec3 N = normalize(Normal_Matrix * vNormal);
  
	vec4 ambient = AmbientProduct;
	
	float d = max( dot(L, N), 0.0 );
	vec4  diffuse = d * DiffuseProduct;
	
	float s = pow( max(dot(N, H), 0.0), Shininess );
	vec4  specular = s * SpecularProduct;
	if( dot(L, N) < 0.0 ) {
	specular = vec4(0.0, 0.0, 0.0, 1.0);
	}
	
	vec4 DirColor = 1.0 * (ambient + diffuse + specular);
  
  return DirColor;
  
}

	// Transform vertex  position into eye coordinates
  
vec4 calcPositionalColor(){
    if(EnablePosLight == 0.0){
      return  vec4(0.0, 0.0, 0.0, 0.0);
    }
    
    vec3 pos = (ModelView * vPosition).xyz;
    
    vec3 L = normalize( LightPosition.xyz - pos );
    vec3 E = normalize( -pos );
    vec3 H = normalize( L + E );
    
  vec3 N = normalize(Normal_Matrix * vNormal);
  
    /*--- To Do: Compute attenuation ---*/
    float attenuation = 1.0;
    vec3 tmp_cal_d = LightPosition.xyz - pos;
    float cur_distance=length(tmp_cal_d);
    attenuation=1/(ConstAtt + LinearAtt*cur_distance + QuadAtt*cur_distance*cur_distance);

 // Compute terms in the illumination equation
    vec4 ambient = LightAmbient*MaterialAmbient;
    float d = max( dot(L, N), 0.0 );
    vec4  diffuse = d * LightDiffuse*MaterialDiffuse;
    float s = pow( max(dot(N, H), 0.0), Shininess );
    vec4  specular = s * LightSpecular*MaterialSpecular;
    if( dot(L, N) < 0.0 ) {
	specular = vec4(0.0, 0.0, 0.0, 1.0);
    } 

    float spot_attenuation = 1.0;
    
    if(isPointSource == 0.0){
      vec3 lf = normalize(SpotDirection.xyz - LightPosition.xyz);
      
      spot_attenuation = pow(dot(-L, lf), SpotExp);
      
      if(dot(-L, lf) < SpotAngle){
        spot_attenuation = 0.0;
      }
    }

    /*--- attenuation below must be computed properly ---*/
   vec4 PosColor = spot_attenuation * attenuation * (ambient + diffuse + specular);
   
  return PosColor;

}
