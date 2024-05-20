-- _global

//#extension GL_EXT_shader_image_load_store : require

// for something completely unrelated (PixelSync) - needs deleting from here
//layout(early_fragment_tests) in;

precision highp float;
precision highp int;

//uniform vec4    uScreenSize;
//uniform float roll;

layout(rgba8, binding = 0, location = 0) uniform image2D uTex0;
layout(rgba8, binding = 1, location = 1) uniform image2D uTex1;

const int cKernelSize           = COMPUTE_SHADER_KERNEL_SIZE; // added on the cpp side
const int cKernelHalfDist       = cKernelSize/2;
const float recKernelSize       = 1.0 / float(cKernelSize);

-- ComputeH

layout( local_size_x = CS_THREAD_GROUP_SIZE, local_size_y = 1, local_size_z = 1 ) in;
void main()
{
    int y = int(gl_GlobalInvocationID.x);
    
    // avoid processing pixels that are out of texture dimensions!
    if( y >= (cRTScreenSizeI.w) ) return;

    vec3 colourSum = imageLoad( uTex0, ivec2( 0, y ) ).xyz * float(cKernelHalfDist);
    for( int x = 0; x <= cKernelHalfDist; x++ )
        colourSum += imageLoad( uTex0, ivec2( x, y ) ).xyz;

    for( int x = 0; x < cRTScreenSizeI.z; x++ )
    {
        imageStore( uTex1, ivec2( x, y ), vec4( colourSum * recKernelSize, 1.0 ) );

        // move window to the next 
        vec3 leftBorder     = imageLoad( uTex0, ivec2( max( x-cKernelHalfDist, 0 ), y ) ).xyz;
        vec3 rightBorder    = imageLoad( uTex0, ivec2( min( x+cKernelHalfDist+1, cRTScreenSizeI.z-1 ), y ) ).xyz;

        colourSum -= leftBorder;
        colourSum += rightBorder;
    }
}


-- ComputeV

layout( local_size_x = CS_THREAD_GROUP_SIZE, local_size_y = 1, local_size_z = 1 ) in;
void main()
{
    // x and y are swapped for vertical

    int y = int(gl_GlobalInvocationID.x);

    // avoid processing pixels that are out of texture dimensions!
    if( y >= cRTScreenSizeI.z ) return;

    vec3 colourSum = imageLoad( uTex0, ivec2( y, 0 ) ).xyz * float(cKernelHalfDist);
    for( int x = 0; x <= cKernelHalfDist; x++ )
        colourSum += imageLoad( uTex0, ivec2( y, x ) ).xyz;
    
    for( int x = 0; x < cRTScreenSizeI.w; x++ )
    {
        imageStore( uTex1, ivec2( y, x ), vec4( colourSum * recKernelSize, 1.0 ) );
    
        // move window to the next 
        vec3 leftBorder     = imageLoad( uTex0, ivec2( y, max( x-cKernelHalfDist, 0 ) ) ).xyz;
        vec3 rightBorder    = imageLoad( uTex0, ivec2( y, min( x+cKernelHalfDist+1, cRTScreenSizeI.w-1 ) ) ).xyz;
    
        colourSum -= leftBorder;
        colourSum += rightBorder;
    }
}
