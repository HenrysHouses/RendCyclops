// need to figure out which glsl version i should be using here
layout (location = 0) in vec3 aPos;

void main()
{
    vec4 Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    // gl_Position = Position; 
}
