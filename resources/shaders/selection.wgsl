struct Uniforms {
    proj: mat4x4<f32>,
    canvasPos: vec2<f32>,
    mousePos: vec2<f32>,
    mouseSelectPos: vec2<f32>,
    selectType: u32,
    windowSize: vec2<u32>,
    scale: f32,
};

struct InstanceInput {
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

struct Selection {
    bboxMaxX: f32,
    bboxMaxY: f32,
    bboxMinX: f32,
    bboxMinY: f32,
    flags: u32,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@group(1) @binding(0)
var<storage,read_write> outBuffer: array<Selection>;
@group(1) @binding(1)
var<storage, read> instBuff: array<InstanceInput>;

fn barycentric(v1: vec4<f32>, v2: vec4<f32>, v3: vec4<f32>, p: vec2<f32>) -> vec3<f32> {
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

    let model = mat4x4<f32>(instBuff[layer].basisAX,    instBuff[layer].basisBX,    0.0, instBuff[layer].offsetX,
                            instBuff[layer].basisAY,    instBuff[layer].basisBY,    0.0, instBuff[layer].offsetY,
                            0.0,                        0.0,                        1.0, 0.0,
                            0.0,                        0.0,                        0.0, 1.0);

    // We have 3 possible scenarios with regards to intersection:
    // Ractangle is fully inside selection box
    // Rectangle is fully outside selection box
    // Rectangle is partially inside selection box in which case we need to rasterize

    // We have 3 posible selection modes:
    // mouse select pos == (0.0 0.0) : recalculate bboxes without modifying selection
    // mouse select pos == (1.0 1.0) : use point-click selection
    // mouse select pos is arbitrary size : use bbox selection

    var flags = u32(0);
    var aabb = vec4<f32>(-10000000.0, -10000000.0, 10000000.0, 10000000.0);

    // Find if image is inside selection box
    // Also calculate aabb for image
    for (var i: u32 = 0; i < 4; i = i + 1u) {
        let pos = verts[i] * model;

        aabb.x = max(aabb.x, pos.x);
        aabb.y = max(aabb.y, pos.y);
        aabb.z = min(aabb.z, pos.x);
        aabb.w = min(aabb.w, pos.y);

        if(uniforms.selectType == 0) {
            if(minX < pos.x && pos.x < maxX && minY < pos.y && pos.y < maxY) {
                flags = flags | 1;

            } else {
                flags = flags | 2;
            }
        }
    }

    // Find if mouse is inside image
    if(uniforms.selectType == 1) {
        let mouseBarryA = barycentric(verts[0] * model, verts[1] * model, verts[2] * model, uniforms.mousePos);
        let mouseBarryB = barycentric(verts[0] * model, verts[2] * model, verts[3] * model, uniforms.mousePos);

        if( (0.0 < mouseBarryA.x && mouseBarryA.x < 1.0 && 0.0 < mouseBarryA.y && mouseBarryA.y < 1.0 && 0.0 < mouseBarryA.z && mouseBarryA.z < 1.0 ) ||
            (0.0 < mouseBarryB.x && mouseBarryB.x < 1.0 && 0.0 < mouseBarryB.y && mouseBarryB.y < 1.0 && 0.0 < mouseBarryB.z && mouseBarryB.z < 1.0 )) {
            flags = flags | 1;
        }
    }

    if(uniforms.selectType == 0 || uniforms.selectType == 1) {
        outBuffer[layer].flags = flags;
    }
    outBuffer[layer].bboxMaxX = aabb.x;
    outBuffer[layer].bboxMaxY = aabb.y;
    outBuffer[layer].bboxMinX = aabb.z;
    outBuffer[layer].bboxMinY = aabb.w;
}
