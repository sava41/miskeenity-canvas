struct VertexInput {
    @builtin(vertex_index) vertexId: u32,
    @builtin(instance_index) instanceId: u32,
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
};

struct InstanceInput {
    @location(2)    offsetX: f32,
    @location(3)    offsetY: f32,
    @location(4)    basisAX: f32,
    @location(5)    basisAY: f32,
    @location(6)    basisBX: f32,
    @location(7)    basisBY: f32,
    @location(8)    uvTop: vec2<u32>,
    @location(9)    uvBot: vec2<u32>,
    @location(10)   imageMaskIds: vec2<u32>,
    @location(11)   color: vec4<u32>,
    @location(12)   flags: u32,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec4<f32>,
    @location(2) @interpolate(flat) inst: u32,
};

struct Uniforms {
    proj: mat4x4<f32>,
    canvasPos: vec2<f32>,
    mousePos: vec2<f32>,
    mouseSelectPos: vec2<f32>,
    windowSize: vec2<u32>,
    scale: f32,
    selectType: u32,
};

struct Selection {
    bboxMaxX: f32,
    bboxMaxY: f32,
    bboxMinX: f32,
    bboxMinY: f32,
    flags: u32,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var texture: texture_2d<f32>;

@vertex
fn vs_main(vert: VertexInput, inst: InstanceInput) -> VertexOutput {
    var out: VertexOutput;

    let model = mat4x4<f32>(inst.basisAX, inst.basisBX, 0.0, inst.offsetX,
                            inst.basisAY, inst.basisBY, 0.0, inst.offsetY,
                            0.0,            0.0,            1.0, 0.0,
                            0.0,            0.0,            0.0, 1.0);

    out.position = vec4<f32>(vert.position, 0.0, 1.0) * model * uniforms.proj ;
    out.uv = vert.uv;
    out.inst = vert.instanceId;

    out.color = vec4<f32>(f32(inst.color.r) / 255.0, f32(inst.color.g) / 255.0, f32(inst.color.b) / 255.0, 1.0) +
                (vec4<f32>(vert.uv, 1.0, 1.0) * 0.3 - 0.15) * f32(inst.flags & 1);

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let texColor = textureSample(texture, textureSampler, in.uv).rgb;

    return vec4f(texColor, in.color.a);
}