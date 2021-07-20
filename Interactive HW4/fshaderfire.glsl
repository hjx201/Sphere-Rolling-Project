/*****************************
 * File: fshaderfire.glsl
 *       A simple fragment shader
 *****************************/

// #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

in  vec4 color;
out vec4 fColor;

in float y; 
void main() 
{ 
    if(y < 0.1) discard;
    fColor = color;
} 

