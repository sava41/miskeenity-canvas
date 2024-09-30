struct VertexInput {
    @builtin(vertex_index) vertexId: u32,
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) size: vec2<f32>,
    @location(3) color: vec4<f32>,
    @location(4) layer: u32,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec4<f32>,
    @location(2) outlineColor: vec4<f32>,
    @location(3) size: vec2<f32>,
    @location(4) @interpolate(flat) flags: u32,
    @location(5) outlineValue: f32,
    @location(6) sdfSize: f32,
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
    numLayers: u32,
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
    color: u32,
    flags: u32,
    meshOffsetLength: u32,
    imageMaskIds: u32,

    extra0: u32,
    extra1: u32,
    extra2: u32,
    extra3: u32,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;
@group(0) @binding(1)
var<storage,read> layerBuff: array<Layer>;

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var texture: texture_2d<f32>;

@group(2) @binding(0) var maskSampler: sampler;
@group(2) @binding(1) var mask: texture_2d<f32>;

fn U32toVec2(a: u32)->vec2<u32> {
    return vec2(u32(( a >> 0 ) & 0xFFFF ), u32(( a >> 16 ) & 0xFFFF ));
}

fn u32toVec4(a: u32)->vec4<u32> {
    return vec4(u32(( a >> 0 ) & 0xFF ), u32(( a >> 8 ) & 0xFF ), u32(( a >> 16 ) & 0xFF ), u32(( a >> 24 ) & 0xFF ));
}


@vertex
fn vs_main(vert: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    out.position = vec4<f32>(vert.position, 0.0, 1.0) * uniforms.proj ;
    out.uv = vert.uv;
    out.size = vert.size;
    out.flags = layerBuff[vert.layer].flags;
    out.color = vert.color;
    out.outlineColor = select(vert.color, vec4<f32>(u32toVec4(layerBuff[vert.layer].extra0)) / 255.0, bool(layerBuff[vert.layer].flags & (1 << 3)));
    out.outlineValue = select(0.0, bitcast<f32>(layerBuff[vert.layer].extra1), bool(layerBuff[vert.layer].flags & (1 << 3)));
    out.sdfSize = bitcast<f32>(layerBuff[vert.layer].extra2);

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

    let maskValue: f32 = select(1.0, textureSample(mask, maskSampler, in.uv).r, bool(in.flags & (1 << 2)) );

    let smoothing: f32 =  1.0 / (16.0 * in.sdfSize * uniforms.scale );
    let sdfMask:  f32 = select(1.0, smoothstep(max(0.05, 0.5 - in.outlineValue - smoothing), 0.5 - in.outlineValue + smoothing, textureSample(mask, maskSampler, in.uv).r), bool(in.flags & (1 << 3)));
    let sdfOutlineColor: vec3<f32> = mix(in.outlineColor.rgb, in.color.rgb, smoothstep(0.5 - smoothing, 0.5 + smoothing, textureSample(mask, maskSampler, in.uv).r));

    let selectColor: f32 = f32(((modf(in.position.x / f32(15.0) + f32(modf(in.position.y / f32(15.0)).whole % 2)).whole % 2) * 0.1 - 0.05) * f32(in.flags & 1));

    let pillMask: f32 = select(1.0, smoothstep(0.0, 2.0 / (max(in.size.x, in.size.y) * uniforms.scale) , -udRoundedBox((in.uv - 0.5) * aspect, vec2<f32>(0.5) * aspect, 0.5f)), bool(in.flags & (1 << 4)));

    return vec4<f32>(texColor.rgb * select(in.color.rgb, sdfOutlineColor, bool(in.flags & (1 << 3)) && in.outlineValue > 0.0) + vec3<f32>(selectColor), texColor.a * maskValue * sdfMask * pillMask * in.color.a);
}