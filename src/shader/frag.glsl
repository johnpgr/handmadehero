#version 450 core

in vec2 TexCoord;
out vec4 FragColor;

uniform float uTime;
uniform vec2 uResolution;

void main()
{
    // Get pixel coordinates (similar to Casey's XOffset, YOffset)
    vec2 pixelCoord = TexCoord * uResolution;
    
    // Create the weird gradient effect like in Handmade Hero
    // This mimics Casey's gradient calculation
    float x = pixelCoord.x;
    float y = pixelCoord.y;
    
    // Casey's gradient formula adapted for normalized values
    float red = (sin(uTime * 0.5) + 1.0) * 0.5; // Animate red component
    float green = x / uResolution.x; // Horizontal gradient
    float blue = y / uResolution.y;  // Vertical gradient
    
    // Add some time-based animation like Casey's moving gradient
    red = (sin(x * 0.01 + uTime) + 1.0) * 0.5;
    green = (sin(y * 0.01 + uTime * 0.7) + 1.0) * 0.5;
    blue = (sin((x + y) * 0.005 + uTime * 0.3) + 1.0) * 0.5;
    
    FragColor = vec4(red, green, blue, 1.0);
}
