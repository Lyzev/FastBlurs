


-- Vertex

in vec2 Position;

void main()
{
    gl_Position = vec4( Position.x, Position.y, 0.0, 1.0 );
}


-- Fragment2x2
precision highp float;
out vec4 FragColor;

uniform vec3 uDiffuseMaterial;
uniform vec3 uAmbientMaterial;

uniform sampler2D uTex0;

//layout(origin_upper_left) 
//in highp vec4 gl_FragCoord;

void main()
{
    vec3 col = texture(uTex0, gl_FragCoord.xy * cRTPixelSize.zw).xyz;
    FragColor = vec4( col.xyz, 1.0 );
}


-- Fragment4x4
precision highp float;
out vec4 FragColor;

uniform vec3 uDiffuseMaterial;
uniform vec3 uAmbientMaterial;

uniform sampler2D uTex0;

//layout(origin_upper_left) 
//in highp vec4 gl_FragCoord;

void main()
{
    vec2 vTexCoord = gl_FragCoord.xy * cRTPixelSize.zw;

    // need to use textureOffset here
    vec3 col0 = textureOffset(uTex0, vTexCoord, ivec2( -2,  0 ) ).xyz;
    vec3 col1 = textureOffset(uTex0, vTexCoord, ivec2(  2,  0 ) ).xyz;
    vec3 col2 = textureOffset(uTex0, vTexCoord, ivec2(  0, -2 ) ).xyz;
    vec3 col3 = textureOffset(uTex0, vTexCoord, ivec2(  0,  2 ) ).xyz;

    vec3 col = (col0+col1+col2+col3) * 0.25;

    FragColor = vec4( col.xyz, 1.0 );
}

