struct VertexInput {
    @builtin(vertex_index) vertexId: u32,
    @builtin(instance_index) instanceId: u32,
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
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

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var texture: texture_2d<f32>;

fn colorU32tovec4(c: u32)->vec4<f32> {
    let s = f32(1.0f / 255.0f);

    return vec4(f32(( c >> 0 ) & 0xFF ) * s ,f32(( c >> 8 ) & 0xFF ) * s ,f32(( c >> 16 ) & 0xFF ) * s ,f32(( c >> 24 ) & 0xFF ) * s);
}

@vertex
fn vs_main(vert: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    let model = mat4x4<f32>(layerBuff[vert.instanceId].basisAX, layerBuff[vert.instanceId].basisBX, 0.0, layerBuff[vert.instanceId].offsetX,
                            layerBuff[vert.instanceId].basisAY, layerBuff[vert.instanceId].basisBY, 0.0, layerBuff[vert.instanceId].offsetY,
                            0.0,            0.0,            1.0, 0.0,
                            0.0,            0.0,            0.0, 1.0);

    out.position = vec4<f32>(vert.position, 0.0, 1.0) * model * uniforms.proj ;
    out.uv = vert.uv;
    out.flags = layerBuff[vert.instanceId].flags;
    
    out.size.x = length(vec2<f32>(layerBuff[vert.instanceId].basisAX, layerBuff[vert.instanceId].basisAY));
    out.size.y = length(vec2<f32>(layerBuff[vert.instanceId].basisBX, layerBuff[vert.instanceId].basisBY));
    
    out.screenSize.x = length(vec4<f32>(layerBuff[vert.instanceId].basisAX - uniforms.canvasPos.x, layerBuff[vert.instanceId].basisAY - uniforms.canvasPos.x, 0.0, 1.0)* uniforms.proj);
    out.screenSize.y = length(vec4<f32>(layerBuff[vert.instanceId].basisBX - uniforms.canvasPos.x, layerBuff[vert.instanceId].basisBY - uniforms.canvasPos.y, 0.0, 1.0)* uniforms.proj);

    out.color = colorU32tovec4(layerBuff[vert.instanceId].color);

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