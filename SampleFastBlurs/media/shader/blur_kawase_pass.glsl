


-- Vertex

in vec2 Position;
out vec2 vTexCoord;

void main()
{
    gl_Position = vec4( Position.x, Position.y, 0.0, 1.0 );
    vTexCoord = vec2( Position.x * 0.5 + 0.5, Position.y * 0.5 + 0.5 );
}


-- Fragment
precision highp float;
out vec4 FragColor;
in vec2 vTexCoord;

uniform vec3 uDiffuseMaterial;
uniform vec3 uAmbientMaterial;

uniform sampler2D uTex0;

uniform vec4 u_xyPixelSize_zIteration_wDummy;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Developed by Masaki Kawase, Bunkasha Games
// Used in DOUBLE-S.T.E.A.L. (aka Wreckless)
// From his GDC2003 Presentation: Frame Buffer Postprocessing Effects in  DOUBLE-S.T.E.A.L (Wreckless)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
vec3 KawaseBlurFilter( sampler2D tex, vec2 texCoord, vec2 pixelSize, float iteration )
{
    vec2 texCoordSample;
    vec2 halfPixelSize = pixelSize / 2.0f;
    vec2 dUV = ( pixelSize.xy * vec2( iteration, iteration ) ) + halfPixelSize.xy;

    vec3 cOut;

    // Sample top left pixel
    texCoordSample.x = texCoord.x - dUV.x;
    texCoordSample.y = texCoord.y + dUV.y;
    
    cOut = texture( tex, texCoordSample ).xyz;

    // Sample top right pixel
    texCoordSample.x = texCoord.x + dUV.x;
    texCoordSample.y = texCoord.y + dUV.y;

    cOut += texture( tex, texCoordSample ).xyz;

    // Sample bottom right pixel
    texCoordSample.x = texCoord.x + dUV.x;
    texCoordSample.y = texCoord.y - dUV.y;
    cOut += texture( tex, texCoordSample ).xyz;

    // Sample bottom left pixel
    texCoordSample.x = texCoord.x - dUV.x;
    texCoordSample.y = texCoord.y - dUV.y;

    cOut += texture( tex, texCoordSample ).xyz;

    // Average 
    cOut *= 0.25f;
    
    return cOut;
}

void main()
{
    //vec3 col = texture(uTex0, vTexCoord).xyz;
    FragColor.xyz = KawaseBlurFilter( uTex0, vTexCoord.xy, u_xyPixelSize_zIteration_wDummy.xy, u_xyPixelSize_zIteration_wDummy.z );

    // // double-Kawase is also an option, but loses some quality
    // FragColor.xyz += KawaseBlurFilter( uTex0, vTexCoord.xy, u_xyPixelSize_zIteration_wDummy.xy, u_xyPixelSize_zIteration_wDummy.z*2.0 + 1.0 );
    // FragColor.xyz *= 0.5;

    FragColor.w = 1.0;
}

