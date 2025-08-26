// shockwave.fs
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution; // screen resolution
uniform vec2 clickPos;   // normalized click position (0..1)
uniform float strength;  // how strong compression is

float calculateOffset(vec2 dir, float maxRadius, float t){
    float d = length(dir) - t*maxRadius;
    d*= 1.0 - smoothstep(0.0,0.05, abs(d));
    
    //Intro and Outro
    d*= smoothstep(0.0, 0.05, t);
    d*= 1.0-smoothstep(0.5, 1.0, t);
    return d;
}

void main()
{
    
    //float t = pow(iTime*0.1, 1.0/1.5);
    float t = pow(strength, 1.0);
    const float maxRadius = 1;
    
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragTexCoord;
    vec2 centr = clickPos;
    vec2 dir = centr - uv;
    dir.x*=resolution.x/resolution.y;
    
    float rd = calculateOffset(dir, maxRadius, t-0.02);
    float gd = calculateOffset(dir, maxRadius, t);
    float bd = calculateOffset(dir, maxRadius, t+0.02);
    dir = normalize(dir);
    

    // Time varying pixel color
    //vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));
    float r = texture(texture0, uv+dir*rd).r;
    float b = texture(texture0, uv+dir*bd).b;
    float g = texture(texture0, uv+dir*gd).g;
    
    vec3 color = vec3(r,g,b);
    color.rgb+=gd*8.0;

    // Output to screen
    finalColor = vec4(color.rgb,1.0);
}