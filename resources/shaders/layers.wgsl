struct VertexInput {
    @builtin(vertex_index) vertexId: u32,
    @builtin(instance_index) instanceId: u32,
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
};

struct InstanceInput {
    @location(2) offset: vec2<f32>,
    @location(3) basis_a: vec2<f32>,
    @location(4) basis_b: vec2<f32>,
    @location(5) uv_top: vec2<u32>,
    @location(6) uv_bot: vec2<u32>,
    @location(7) image_mask_ids: vec2<u32>,
    @location(8) color_type: vec4<u32>,
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
@group(0) @binding(1)
var<storage,read_write> selectionData: array<Selection>;

@vertex
fn vs_main(vert: VertexInput, inst: InstanceInput) -> VertexOutput {
    var out: VertexOutput;

    let model = mat4x4<f32>(inst.basis_a.x, inst.basis_b.x, 0.0, inst.offset.x,
                            inst.basis_a.y, inst.basis_b.y, 0.0, inst.offset.y,
                            0.0,            0.0,            1.0, 0.0,
                            0.0,            0.0,            0.0, 1.0);

    out.position = vec4<f32>(vert.position, 0.0, 1.0) * model * uniforms.proj ;
    out.uv = vert.uv;
    out.inst = vert.instanceId;

    out.color = vec4<f32>(f32(inst.color_type.r) / 255.0, f32(inst.color_type.g) / 255.0, f32(inst.color_type.b) / 255.0, 1.0);

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return in.color + (vec4<f32>(in.uv, 1.0, 1.0) * 0.3 - 0.15) * f32(selectionData[in.inst].flags == 1);
}