#version 120

uniform sampler2D texture;
uniform float shieldIntensity;

void main()
{
    vec2 texCoord = gl_TexCoord[0].xy;
    vec4 texColor = texture2D(texture, texCoord);
    
    // If shield is not active, just draw the original sprite
    if (shieldIntensity <= 0.5)
    {
        gl_FragColor = texColor;
        return;
    }
    
    // For outline effect, sample neighboring pixels with a very small step
    // Use a very small step size to avoid sampling from other sprites in the atlas
    float stepSize = 0.0005; // Very small step in normalized coordinates
    
    // Sample in 4 cardinal directions only (to avoid sampling diagonally into other sprites)
    float alphaUp = texture2D(texture, texCoord + vec2(0.0, stepSize)).a;
    float alphaDown = texture2D(texture, texCoord + vec2(0.0, -stepSize)).a;
    float alphaLeft = texture2D(texture, texCoord + vec2(-stepSize, 0.0)).a;
    float alphaRight = texture2D(texture, texCoord + vec2(stepSize, 0.0)).a;
    
    // Check if current pixel is transparent but has opaque neighbors nearby
    // This means we're at the edge of the sprite
    bool shouldDrawOutline = (texColor.a < 0.1) && (alphaUp > 0.5 || alphaDown > 0.5 || alphaLeft > 0.5 || alphaRight > 0.5);
    
    // Brown outline color
    vec3 brownOutline = vec3(0.6, 0.4, 0.2);
    
    // Draw outline only for transparent pixels near opaque edges
    if (shouldDrawOutline)
    {
        gl_FragColor = vec4(brownOutline, 1.0);
    }
    else
    {
        // Draw original sprite color
        gl_FragColor = texColor;
    }
}

