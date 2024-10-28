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
    dpiScale: f32,
    ticks: u32,
};

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
}


@vertex
fn vs_post(@builtin(vertex_index) vertexId : u32) -> VertexOutput {
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

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;
@group(0) @binding(1)
var<storage,read> reserved: array<u32>;

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var texture: texture_2d<f32>;

@group(2) @binding(0) var maskSampler: sampler;
@group(2) @binding(1) var mask: texture_2d<f32>;

@group(3) @binding(0) var maskOccludedSampler: sampler;
@group(3) @binding(1) var maskOccluded: texture_2d<f32>;

const samples = u32(12);
const outlineWidth = f32(1.0);
const orange600 = vec4<f32>(0.97647, 0.64314, 0.24706, 1.0);
const gray100 = vec4<f32>(0.19608, 0.19608, 0.19608, 1.0);
const pi = radians(180.0);
@fragment
fn fs_post(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32> {
    
    //search in a circle if we are on the edge
    var onEdge: bool = false;
    var test: f32 = 0.0;
    for(var i: u32 = 0; i < samples; i = i + 1u)
    {
        let angle: f32 = f32(i)/f32(samples) * 2.0 * pi;
        let sampleUv: vec2<f32> = uv + vec2<f32>(cos(angle) / f32(uniforms.windowWidth), sin(angle) / f32(uniforms.windowHeight)) * outlineWidth * uniforms.dpiScale;
        let testValue: vec4<f32> = textureSample(mask, maskSampler, sampleUv);
        onEdge = onEdge || (testValue.r < 1.0);
    }

    let diagonals: u32 = (u32(uv.x * f32(uniforms.windowWidth)) - u32(uv.y * f32(uniforms.windowHeight)) + uniforms.ticks / 50) & 8;
    let diagonalsColored: vec4<f32> = mix(orange600, gray100, clamp(f32(diagonals), 0.0, 1.0));

    return mix(textureSample(texture, textureSampler, uv), diagonalsColored, clamp(f32(onEdge) - (1.0 - textureSample(maskOccluded, maskOccludedSampler, uv).r), 0.0, 1.0));
}