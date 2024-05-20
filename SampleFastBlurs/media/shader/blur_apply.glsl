
-- Vertex

in vec2 Position;

void main()
{
    gl_Position = vec4( Position.x, Position.y, 0.0, 1.0 );
}


-- Fragment
precision highp float;
out vec4 FragColor;

uniform sampler2D uTex0;

//layout(origin_upper_left) 
//in highp vec4 gl_FragCoord;

uniform vec4 uApplyParams;

uniform vec4 uFinalPixelSize;

void main()
{
    vec3 col = texture( uTex0, gl_FragCoord.xy * uFinalPixelSize.xy ).xyz;

    float lum = dot( col, vec3( 0.299, 0.587, 0.114 ) );

    float bluoum = clamp(lum - uApplyParams.y, 0.0, 1.0 ) * uApplyParams.x;

    col.xyz += vec3( bluoum, bluoum, bluoum );

    FragColor = vec4( col.xyz, 1.0 );
}

