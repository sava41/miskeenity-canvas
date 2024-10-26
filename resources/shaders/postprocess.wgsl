struct Uniforms {
    proj: mat4x4<f32>,
    canvasPos: vec2<f32>,
    mousePos: vec2<f32>,
    mouseSelectPos: vec2<f32>,
    selectType: u32,
    viewType: u32,
    windowWidth: u32,
    windowHeight: u32,
    scale: f32,
    numLayers: u32,
};

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
}


@vertex
fn vs_post(@builtin(vertex_index) vertexId : u32) -> VertexOutput {
    const pos = array(
          vec2( 1.0,  1.0),
          vec2( 1.0, -1.0),
          vec2(-1.0, -1.0),
          vec2( 1.0,  1.0),
          vec2(-1.0, -1.0),
          vec2(-1.0,  1.0),
        );
      
        const uv = array(
          vec2(1.0, 0.0),
          vec2(1.0, 1.0),
          vec2(0.0, 1.0),
          vec2(1.0, 0.0),
          vec2(0.0, 1.0),
          vec2(0.0, 0.0),
        );

        var output : VertexOutput;
        output.position = vec4(pos[vertexId], 0.0, 1.0);
        output.uv = uv[vertexId];
        return output;
}

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;
@group(0) @binding(1)
var<storage,read> reserved: array<u32>;

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var texture: texture_2d<f32>;

@fragment
fn fs_post(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32> {

    return textureSample(texture, textureSampler, uv);
}