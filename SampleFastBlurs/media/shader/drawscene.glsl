


-- Vertex

in vec2 Position;
in vec4 Color;

//in vec2 TexCoord;
//out vec2 vTexCoord;
out vec4 vColor;

void main()
{
    gl_Position = vec4( Position.x, Position.y, 0.0, 1.0 );
    //vTexCoord = vec2( Position.x * 0.5 + 0.5, -Position.y * 0.5 + 0.5 );
    vColor = Color;
}

-- Fragment
precision highp float;
out vec4 FragColor;
//in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uDiffuseMaterial;
uniform vec3 uAmbientMaterial;

uniform sampler2D uTex0;

void main()
{
    //float k = uDiffuseMaterial.x + uAmbientMaterial.x;
    //vec3 col = texture(uTex0, vTexCoord).xyz;
    FragColor = vColor;
}

