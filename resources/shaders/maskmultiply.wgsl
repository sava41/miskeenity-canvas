@group(0) @binding(0) var textureSamplerInput: sampler;
@group(0) @binding(1) var inputTexture: texture_2d<f32>;

@group(1) @binding(0) var textureSamplerMask: sampler;
@group(1) @binding(1) var inputMask: texture_2d<f32>;

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
}

@vertex
fn vs_cut(@builtin(vertex_index) vertexId : u32) -> VertexOutput {
    const pos = array(
          vec2<f32>( 1.0,  1.0),
          vec2<f32>( 1.0, -1.0),
          vec2<f32>(-1.0, -1.0),
          vec2<f32>( 1.0,  1.0),
          vec2<f32>(-1.0, -1.0),
          vec2<f32>(-1.0,  1.0),
        );
      
    const uv = array(
          vec2<f32>(1.0, 0.0),
          vec2<f32>(1.0, 1.0),
          vec2<f32>(0.0, 1.0),
          vec2<f32>(1.0, 0.0),
          vec2<f32>(0.0, 1.0),
          vec2<f32>(0.0, 0.0),
        );

    var output : VertexOutput;
    output.position = vec4(pos[vertexId], 0.0, 1.0);
    output.uv = uv[vertexId];
    return output;
}

struct FragmentOutput {
    @location(0) outputA: vec4<f32>,
    @location(1) outputB: vec4<f32>,
}

@fragment
fn fs_cut(@location(0) uv : vec2<f32>) ->  FragmentOutput {
    
    let input: vec4<f32> = textureSample(inputTexture, textureSamplerInput, uv);
    let mask: vec4<f32> = textureSample(inputMask, textureSamplerMask, uv);
    
    var out: FragmentOutput;
    out.outputA = vec4<f32>(input.rgb, input.a * mask.r);
    out.outputB = vec4<f32>(input.rgb, input.a * (1.0 - mask.r));

    return out;
}