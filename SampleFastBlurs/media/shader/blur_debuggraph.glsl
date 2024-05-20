
-- Vertex
in vec2 Position;

void main()
{
    gl_Position = vec4( Position.x, Position.y, 0.0, 1.0 );
}


-- FragDrawInput
precision highp float;
out vec4 FragColor;

//layout(origin_upper_left) 
//in highp vec4 gl_FragCoord;

void main()
{
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);

    const int rectHalfSize = KERNEL_SIZE_FOR_DEBUG_GRAPH;
    const int kernelSize = KERNEL_SIZE_FOR_DEBUG_GRAPH*2 + rectHalfSize;

    if( (float(fragCoord.x) > (float(kernelSize+64)*4.0) ) || (float(fragCoord.y) > (float(kernelSize+64)*4.0)) )
        discard;

    FragColor.xyz = vec3( 0.0, 0.0, 0.0 );

    if( ((fragCoord.x + rectHalfSize) >= kernelSize) && ((fragCoord.x - rectHalfSize) < kernelSize) 
     && ((fragCoord.y + rectHalfSize) >= kernelSize) && ((fragCoord.y - rectHalfSize) < kernelSize) )
        FragColor.xyz = vec3( 1.0, 1.0, 1.0 );

    FragColor.w = 1.0;
}

-- FragDrawGraph
precision highp float;

out vec4 FragColor;

uniform sampler2D uTex0;
uniform sampler2D uTex1;

//layout(origin_upper_left) 
//in highp vec4 gl_FragCoord;

void main()
{
    const int rectHalfSize = KERNEL_SIZE_FOR_DEBUG_GRAPH;
    const int kernelSize = KERNEL_SIZE_FOR_DEBUG_GRAPH*2 + rectHalfSize;

        // some lesser GLSL compiler don't honor curly brackets for multiple variable definitions with same name in different scopes, so define them once outside
        float stripeEveryPixels;
        vec2 tmp1;
        vec2 tmp2;

    // first graph
    {
        ivec2 graphScreenOffset = ivec2( 10, 140 );
        ivec2 graphScreenSize   = ivec2( 500, 500 );

        vec2 graphSrcSize = vec2( kernelSize*2, kernelSize*2 );

        FragColor.a = 0.0;

        ivec2 fragCoord = ivec2(gl_FragCoord.xy);
        fragCoord -= graphScreenOffset;
    
        if( !( (fragCoord.x < 0) || (fragCoord.x > graphScreenSize.x ) || (fragCoord.y < 0) || (fragCoord.y > graphScreenSize.y ) ) )
        {
            // calc coords
            vec2 srcUVInGraph = (vec2(fragCoord.xy)+vec2(0.5, 0.5)) / vec2(graphScreenSize);
            vec2 srcUV = srcUVInGraph * ( graphSrcSize / cRTScreenSize.xy );
    
            // sample line in the middle of the kernel from the last blurred texture
            vec3 col = texture( uTex0, vec2( srcUV.x, graphSrcSize.y * 0.5 / cRTScreenSize.y ) ).xyz;
    
            // convert to graph
            col = clamp( ((col.xyz+0.01) - (srcUVInGraph.yyy*1.02)) * (vec3(graphScreenSize.yyy) * 0.5), vec3(0.0, 0.0, 0.0), vec3( 1.0, 1.0, 1.0 ) );

            bool marked = false;

            if( !marked )
            {
                stripeEveryPixels = 8.0;
                tmp1 = ((vec2(fragCoord.xy)+vec2(0.0, 0.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                tmp2 = ((vec2(fragCoord.xy)+vec2(1.0, 1.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                if( floor( tmp1.x ) != floor( tmp2.x ) )
                {   
                    col += vec3( 0.3, -0.3, -0.3 );
                    marked = true;
                }
            }
            if( !marked )
            {
                stripeEveryPixels = 2.0;
                tmp1 = ((vec2(fragCoord.xy)+vec2(0.0, 0.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                tmp2 = ((vec2(fragCoord.xy)+vec2(1.0, 1.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                if( floor( tmp1.x ) != floor( tmp2.x ) )
                {   
                    col += vec3( -0.3, -0.3, 0.3 );
                    marked = true;
                }
            }
            if( !marked )
            {
                stripeEveryPixels = 1.0;
                tmp1 = ((vec2(fragCoord.xy)+vec2(0.0, 0.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                tmp2 = ((vec2(fragCoord.xy)+vec2(1.0, 1.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                if( (floor( tmp1.x ) != floor( tmp2.x )) && (srcUVInGraph.y<=0.02) )
                {
                    col += vec3( -0.3, 0.3, -0.3 );
                    marked = true;
                }
            }

            if( (fragCoord.x <= 0) || (fragCoord.x >= graphScreenSize.x ) || (fragCoord.y <= 0) || (fragCoord.y >= graphScreenSize.y ) )
                col = vec3( 0.0, 0.0, 1.0 );

            FragColor.xyz = col;
            FragColor.w = 1.0;
        }
    }

    // second graph
    if( FragColor.a == 0.0 )
    {
        ivec2 graphScreenOffset = ivec2( 520, 140 );
        ivec2 graphScreenSize   = ivec2( 500, 500 );

        vec2 graphSrcSize = vec2( kernelSize*2, kernelSize*2 );

        FragColor.a = 0.0;

        ivec2 fragCoord = ivec2(gl_FragCoord.xy);
        fragCoord -= graphScreenOffset;
    
        if( !( (fragCoord.x < 0) || (fragCoord.x > graphScreenSize.x ) || (fragCoord.y < 0) || (fragCoord.y > graphScreenSize.y ) ) )
        {
            // calc coords
            vec2 srcUVInGraph = (vec2(fragCoord.xy)+vec2(0.5, 0.5)) / vec2(graphScreenSize);
            vec2 srcUV = srcUVInGraph * ( graphSrcSize / cRTScreenSize.xy );
    
            // sample line in the middle of the kernel from the last blurred texture
            vec3 col = texture( uTex1, vec2( srcUV.xy ) ).xyz;

            bool marked = false;

            // Isohypses
            {
                float isohypseStep = 0.1;

                float cc = col.x / isohypseStep;
                float c0 = texture( uTex1, (vec2(fragCoord.xy)+vec2(-0.5, 0.5)) / vec2(graphScreenSize) * ( graphSrcSize / cRTScreenSize.xy ) ).x / isohypseStep;
                //float c1 = texture( uTex1, (vec2(fragCoord.xy)+vec2(+1.5, 0.5)) / vec2(graphScreenSize) * ( graphSrcSize / cRTScreenSize.xy ) ).x / isohypseStep;
                float c2 = texture( uTex1, (vec2(fragCoord.xy)+vec2(0.5, -0.5)) / vec2(graphScreenSize) * ( graphSrcSize / cRTScreenSize.xy ) ).x / isohypseStep;
                //float c3 = texture( uTex1, (vec2(fragCoord.xy)+vec2(0.5, +1.5)) / vec2(graphScreenSize) * ( graphSrcSize / cRTScreenSize.xy ) ).x / isohypseStep;

                if( ( floor( cc ) != floor( c0 ) ) || ( floor( cc ) != floor( c2 ) ) ) //||  ( floor( cc ) != floor( c1 ) ) || ( floor( cc ) != floor( c3 ) ) )
                {
                    col += vec3( -0.5, 0.5, -0.5 );
                    marked = false;
                }
            }

            if( !marked )
            {
                stripeEveryPixels = 8.0;
                tmp1 = ((vec2(fragCoord.xy)+vec2(0.0, 0.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                tmp2 = ((vec2(fragCoord.xy)+vec2(1.0, 1.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                if( (floor( tmp1.x ) != floor( tmp2.x )) || (floor( tmp1.y ) != floor( tmp2.y )) )
                {   
                    col += vec3( 0.3, -0.3, -0.3 );
                    marked = true;
                }
            }
            if( !marked )
            {
                stripeEveryPixels = 2.0;
                tmp1 = ((vec2(fragCoord.xy)+vec2(0.0, 0.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                tmp2 = ((vec2(fragCoord.xy)+vec2(1.0, 1.0)) / vec2(graphScreenSize)) * graphSrcSize / vec2( stripeEveryPixels, stripeEveryPixels );
                if( (floor( tmp1.x ) != floor( tmp2.x )) || (floor( tmp1.y ) != floor( tmp2.y )) )
                {   
                    col += vec3( -0.3, -0.3, 0.3 );
                    marked = true;
                }
            }

            if( (fragCoord.x <= 0) || (fragCoord.x >= graphScreenSize.x ) || (fragCoord.y <= 0) || (fragCoord.y >= graphScreenSize.y ) )
                col = vec3( 0.0, 0.8, 0.0 );

            FragColor.xyz = col;
            FragColor.w = 1.0;
        }
    }

    if( FragColor.a == 0.0 )
        discard;
}

