
struct Uniforms {
    proj: mat4x4<f32>,
    canvasPos: vec2<f32>,
    mousePos: vec2<f32>,
    mouseSelectPos: vec2<f32>,
    selectType: u32,
    windowSize: vec2<u32>,
    scale: f32,
};

struct Layer {
    offsetX: f32,
    offsetY: f32,
    basisAX: f32,
    basisAY: f32,
    basisBX: f32,
    basisBY: f32,
    uvTop: u32,
    uvBot: u32,
    imageMaskIds: u32,
    color: u32,
    flags: u32,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;
@group(0) @binding(1)
var<storage,read> layerBuff: array<Layer>;

@group(1) @binding(0)
var<storage, read_write> meshBuff: array<vec4<f32>>;
@group(1) @binding(1)
var<storage, read_write> vertexBuff: array<vec4<f32>>;

@compute @workgroup_size(256, 1)
fn ma_main(@builtin(global_invocation_id) id_global : vec3<u32>, @builtin(local_invocation_id) id_local : vec3<u32>) {
    let i = u32(id_global.x) * 3;
    
    vertexBuff[i] = meshBuff[i];
    vertexBuff[i + 1] = meshBuff[i+1];
    vertexBuff[i + 2] = meshBuff[i+2];
}