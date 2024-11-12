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

struct MeshVertex {
    xy: vec2<f32>,
    uv: vec2<f32>,
    size: vec2<f32>,
    color: u32,
    padding: u32,
}

struct Vertex {
    xy: vec2<f32>,
    uv: vec2<f32>,
    size: vec2<f32>,
    color: u32,
    layer: u32,
};

struct Selection {
    bboxMaxX: f32,
    bboxMaxY: f32,
    bboxMinX: f32,
    bboxMinY: f32,
    flags: u32,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<storage,read> layerBuff: array<Layer>;

@group(1) @binding(0) var<storage, read> meshVertexBuff: array<MeshVertex>;
@group(1) @binding(1) var<storage, read_write> vertexBuff: array<Vertex>;

@group(2) @binding(0) var<storage,read_write> outBuffer: array<Selection>;

fn barycentric(v1: vec2<f32>, v2: vec2<f32>, v3: vec2<f32>, p: vec2<f32>) -> vec3<f32> {
    let u = cross(
        vec3<f32>(v3.x - v1.x, v2.x - v1.x, v1.x - p.x), 
        vec3<f32>(v3.y - v1.y, v2.y - v1.y, v1.y - p.y)
    );

    if (abs(u.z) < 1.0) {
        return vec3<f32>(-1.0, 1.0, 1.0);
    }

    return vec3<f32>(1.0 - (u.x+u.y)/u.z, u.y/u.z, u.x/u.z); 
}

fn u32toVec2(a: u32)->vec2<u32> {
    return vec2(u32(( a >> 0 ) & 0xFFFF ), u32(( a >> 16 ) & 0xFFFF ));
}

@compute @workgroup_size(256, 1)
fn cs_main(@builtin(global_invocation_id) id_global : vec3<u32>, @builtin(local_invocation_id) id_local : vec3<u32>) {
    let i = u32(id_global.x);

    //Find which layer and which triangle within that layer's mesh we're processing
    // var layerIndex =  u32(0);
    // var remainingTris = i;

    // while (layerIndex < uniforms.numLayers && remainingTris >= u32toVec2(layerBuff[layerIndex].meshOffsetLength).y) {
    //     remainingTris -= u32toVec2(layerBuff[layerIndex].meshOffsetLength).y;
    //     layerIndex++;
    // }

    // if (layerIndex >= uniforms.numLayers) {
    //     return;
    // }

    let minX = min(uniforms.mousePos.x, uniforms.mouseSelectPos.x);
    let minY = min(uniforms.mousePos.y, uniforms.mouseSelectPos.y);
    let maxX = max(uniforms.mousePos.x, uniforms.mouseSelectPos.x);
    let maxY = max(uniforms.mousePos.y, uniforms.mouseSelectPos.y);

    // We have 3 possible scenarios with regards to intersection:
    // Triangle is fully inside selection box
    // Triangle is fully outside selection box
    // Triangle is partially inside selection box and uses a texture mask in which case we need to rasterize to determine selection
    // We cant implement mask rasteriaztion right now since we cant bind all mask textures at once so for now use the first two conditions

    // We have 3 posible selection modes:
    // uniforms.selectType = 2 : recalculate aabb's without modifying selection
    // uniforms.selectType = 1 : use point-click selection
    // uniforms.selectType = 0 : use bbox selection

    var flags = u32(0);
    var aabb = vec4<f32>(-10000000.0, -10000000.0, 10000000.0, 10000000.0);

    // Find if triangle is inside selection box
    // Also calculate aabb for triangle
    for (var j: u32 = 0; j < 3; j = j + 1u) {
        let pos = vertexBuff[i * 3 + j].xy;

        aabb.x = max(aabb.x, pos.x);
        aabb.y = max(aabb.y, pos.y);
        aabb.z = min(aabb.z, pos.x);
        aabb.w = min(aabb.w, pos.y);

        if(uniforms.selectType == 0) {
            flags = select(flags | 2, flags | 1, minX < pos.x && pos.x < maxX && minY < pos.y && pos.y < maxY);
        }
    }

    // Find if mouse is inside triangle
    if(uniforms.selectType == 1) {
        let mouseBarryA = barycentric(vertexBuff[i * 3 + 0].xy, vertexBuff[i * 3 + 1].xy, vertexBuff[i * 3 + 2].xy, uniforms.mousePos);

        flags = select(flags, flags | 1, 0.0 < mouseBarryA.x && mouseBarryA.x < 1.0 && 0.0 < mouseBarryA.y && mouseBarryA.y < 1.0 && 0.0 < mouseBarryA.z && mouseBarryA.z < 1.0);
    }

    outBuffer[i].flags = select(outBuffer[i].flags, flags, uniforms.selectType == 0 || uniforms.selectType == 1);
    
    outBuffer[i].bboxMaxX = aabb.x;
    outBuffer[i].bboxMaxY = aabb.y;
    outBuffer[i].bboxMinX = aabb.z;
    outBuffer[i].bboxMinY = aabb.w;
}