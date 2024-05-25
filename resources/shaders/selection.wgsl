struct Uniforms {
    proj: mat4x4<f32>,
    canvasPos: vec2<f32>,
    mousePos: vec2<f32>,
    mouseSelectPos: vec2<f32>,
    windowSize: vec2<u32>,
    scale: f32,
};

struct InstanceInput {
    offset: vec2<f32>,
    basis_a: vec2<f32>,
    basis_b: vec2<f32>,
    uv_top: u32,
    uv_bot: u32,
    image_mask_ids: u32,
    color_type: u32,
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
var<storage,read_write> outBuffer: array<Selection>;

@group(1) @binding(0)
var<storage, read> instanceBuffer: array<InstanceInput>;

fn barycentric(v1: vec3<f32>, v2: vec3<f32>, v3: vec3<f32>, p: vec2<f32>) -> vec3<f32> {
    let u = cross(
        vec3<f32>(v3.x - v1.x, v2.x - v1.x, v1.x - p.x), 
        vec3<f32>(v3.y - v1.y, v2.y - v1.y, v1.y - p.y)
    );

    if (abs(u.z) < 1.0) {
        return vec3<f32>(-1.0, 1.0, 1.0);
    }

    return vec3<f32>(1.0 - (u.x+u.y)/u.z, u.y/u.z, u.x/u.z); 
}

@compute @workgroup_size(256, 1)
fn cs_main(@builtin(global_invocation_id) id_global : vec3<u32>, @builtin(local_invocation_id) id_local : vec3<u32>) {
    let layer = u32(id_global.x);
    
    let verts = array<vec4<f32>, 4>(
    vec4<f32>( -0.5,  -0.5, 0.0, 1.0),
    vec4<f32>(  0.5,  -0.5, 0.0, 1.0),
    vec4<f32>(  0.5,   0.5, 0.0, 1.0),
    vec4<f32>( -0.5,   0.5, 0.0, 1.0));

    let minX = min(uniforms.mousePos.x, uniforms.mouseSelectPos.x);
    let minY = min(uniforms.mousePos.y, uniforms.mouseSelectPos.y);
    let maxX = max(uniforms.mousePos.x, uniforms.mouseSelectPos.x);
    let maxY = max(uniforms.mousePos.y, uniforms.mouseSelectPos.y);

    let model = mat4x4<f32>(instanceBuffer[layer].basis_a.x,
                            instanceBuffer[layer].basis_b.x, 0.0,
                            instanceBuffer[layer].offset.x,
                            instanceBuffer[layer].basis_a.y,
                            instanceBuffer[layer].basis_b.y, 0.0,
                            instanceBuffer[layer].offset.y,
                            0.0, 0.0, 1.0, 0.0,
                            0.0, 0.0, 0.0, 1.0);

    // We have 3 possible scenarios:
    // Ractangle is fully inside selection box
    // Rectangle is fully outside selection box
    // Rectangle is partially inside selection box in which case we need to rasterize

    var flags = u32(0);
    var bbox = vec4<f32>(-10000000.0, -10000000.0, 10000000.0, 10000000.0);

    for (var i: u32 = 0; i < 4; i = i + 1u) {
        let pos = verts[i] * model;

        bbox.x = max(bbox.x, pos.x);
        bbox.y = max(bbox.y, pos.y);
        bbox.z = min(bbox.z, pos.x);
        bbox.w = min(bbox.w, pos.y);

        if(minX < pos.x && pos.x < maxX && minY < pos.y && pos.y < maxY) {
            flags = flags | 1;

        } else {
            flags = flags | 2;
        }
    }

    outBuffer[layer].flags = flags;
    outBuffer[layer].bboxMaxX = bbox.x;
    outBuffer[layer].bboxMaxY = bbox.y;
    outBuffer[layer].bboxMinX = bbox.z;
    outBuffer[layer].bboxMinY = bbox.w;
}
