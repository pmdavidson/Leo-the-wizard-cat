#version 120

uniform sampler2D texture;
uniform float flashIntensity;

void main()
{
    vec4 texColor = texture2D(texture, gl_TexCoord[0].xy);
    
    // Blend with red based on flash intensity
    vec3 redTint = vec3(1.0, 0.0, 0.0);
    vec3 finalColor = mix(texColor.rgb, redTint, flashIntensity);
    
    gl_FragColor = vec4(finalColor, texColor.a);
}

