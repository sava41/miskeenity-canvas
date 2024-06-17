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
    @location(2) size: vec2<f32>,
    @location(3) screenSize: vec2<f32>,
    @location(4) @interpolate(flat) flags: u32,
};

struct Uniforms {
    proj: mat4x4<f32>,
    canvasPos: vec2<f32>,
    mousePos: vec2<f32>,
    mouseSelectPos: vec2<f32>,
    selectType: u32,
    windowWidth: u32,
    windowHeight: u32,
    scale: f32,
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
    out.flags = inst.flags;
    
    out.size.x = length(vec2<f32>(inst.basisAX, inst.basisAY));
    out.size.y = length(vec2<f32>(inst.basisBX, inst.basisBY));
    
    out.screenSize.x = length(vec4<f32>(inst.basisAX - uniforms.canvasPos.x, inst.basisAY - uniforms.canvasPos.x, 0.0, 1.0)* uniforms.proj);
    out.screenSize.y = length(vec4<f32>(inst.basisBX - uniforms.canvasPos.x, inst.basisBY - uniforms.canvasPos.y, 0.0, 1.0)* uniforms.proj);

    out.color = vec4<f32>(f32(inst.color.r) / 255.0, f32(inst.color.g) / 255.0, f32(inst.color.b) / 255.0, 1.0);

    return out;
}

fn sdRoundedBox( p: vec2<f32>, b: vec2<f32>, r: f32 ) -> f32 {
    let q: vec2<f32> = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, vec2<f32>(0.0))) - r;
}

fn udRoundedBox( p: vec2<f32>, b: vec2<f32>, r: f32 ) -> f32
{
    return length(max(abs(p) - b + r, vec2<f32>(0.0))) - r;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let aspect: vec2<f32> = in.size / min(in.size.x, in.size.y);

    let texColor: vec4<f32> = select(vec4<f32>(1.0), textureSample(texture, textureSampler, in.uv).rgba, bool(in.flags & (1 << 1)));

    let selectColor: f32 = f32(((modf(in.position.x / f32(15.0) + f32(modf(in.position.y / f32(15.0)).whole % 2)).whole % 2) * 0.1 - 0.05) * f32(in.flags & 1));

    let pillMask: f32 = select(1.0, smoothstep(0.0, 2.0 / (max(in.size.x, in.size.y) * uniforms.scale) , -udRoundedBox((in.uv - 0.5) * aspect, vec2<f32>(0.5) * aspect, 0.5f)), bool(in.flags & (1 << 4)));

    return vec4<f32>(texColor.rgb * in.color.rgb + vec3<f32>(selectColor), texColor.a * pillMask);
}