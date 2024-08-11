
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

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;
@group(0) @binding(1)
var<storage,read> layerBuff: array<Layer>;

@group(1) @binding(0)
var<storage, read> meshVertexBuff: array<MeshVertex>;
@group(1) @binding(1)
var<storage, read_write> vertexBuff: array<Vertex>;

fn u32toVec2(a: u32)->vec2<u32> {
    return vec2(u32(( a >> 0 ) & 0xFFFF ), u32(( a >> 16 ) & 0xFFFF ));
}

fn multiplyColors(color1: u32, color2: u32) -> u32 {
    let r1 = (color1 >> 24) & 0xFF;
    let g1 = (color1 >> 16) & 0xFF;
    let b1 = (color1 >> 8) & 0xFF;
    let a1 = color1 & 0xFF;

    let r2 = (color2 >> 24) & 0xFF;
    let g2 = (color2 >> 16) & 0xFF;
    let b2 = (color2 >> 8) & 0xFF;
    let a2 = color2 & 0xFF;

    let r = ((r1 * r2) / 255) & 0xFF;
    let g = ((g1 * g2) / 255) & 0xFF;
    let b = ((b1 * b2) / 255) & 0xFF;
    let a = ((a1 * a2) / 255) & 0xFF;

    return (r << 24) | (g << 16) | (b << 8) | a;
}

@compute @workgroup_size(256, 1)
fn ma_main(@builtin(global_invocation_id) id_global : vec3<u32>, @builtin(local_invocation_id) id_local : vec3<u32>) {
    let i = u32(id_global.x);

    //Find which layer and which triangle within that layer's mesh we're processing
    var layerIndex =  u32(0);
    var remainingTris = i;

    while (layerIndex < uniforms.numLayers && remainingTris >= u32toVec2(layerBuff[layerIndex].meshOffsetLength).y) {
        remainingTris -= u32toVec2(layerBuff[layerIndex].meshOffsetLength).y;
        layerIndex++;
    }

    if (layerIndex >= uniforms.numLayers) {
        return;
    }

    let model = mat4x4<f32>(layerBuff[layerIndex].basisAX,  layerBuff[layerIndex].basisBX,  0.0, layerBuff[layerIndex].offsetX,
                            layerBuff[layerIndex].basisAY,  layerBuff[layerIndex].basisBY,  0.0, layerBuff[layerIndex].offsetY,
                            0.0,                            0.0,                            1.0, 0.0,
                            0.0,                            0.0,                            0.0, 1.0);

    let layerSize = vec2<f32>(  length(vec2<f32>(layerBuff[layerIndex].basisAX, layerBuff[layerIndex].basisAY)),
                                length(vec2<f32>(layerBuff[layerIndex].basisBX, layerBuff[layerIndex].basisBY)));
    
    let triIndex = u32toVec2(layerBuff[layerIndex].meshOffsetLength).x + remainingTris;

    vertexBuff[i * 3 + 0].xy = (vec4<f32>(meshVertexBuff[triIndex * 3 + 0].xy, 0.0, 1.0) * model).xy;
    vertexBuff[i * 3 + 0].uv = meshVertexBuff[triIndex * 3 + 0].uv;
    vertexBuff[i * 3 + 0].size = meshVertexBuff[triIndex * 3 + 0].size * layerSize;
    vertexBuff[i * 3 + 0].color = multiplyColors(meshVertexBuff[triIndex * 3 + 0].color, layerBuff[layerIndex].color);
    vertexBuff[i * 3 + 0].layer = layerIndex;

    vertexBuff[i * 3 + 1].xy = (vec4<f32>(meshVertexBuff[triIndex * 3 + 1].xy, 0.0, 1.0) * model).xy;
    vertexBuff[i * 3 + 1].uv = meshVertexBuff[triIndex * 3 + 1].uv;
    vertexBuff[i * 3 + 1].size = meshVertexBuff[triIndex * 3 + 1].size * layerSize;
    vertexBuff[i * 3 + 1].color = multiplyColors(meshVertexBuff[triIndex * 3 + 1].color,  layerBuff[layerIndex].color);
    vertexBuff[i * 3 + 1].layer = layerIndex;

    vertexBuff[i * 3 + 2].xy = (vec4<f32>(meshVertexBuff[triIndex * 3 + 2].xy, 0.0, 1.0) * model).xy;
    vertexBuff[i * 3 + 2].uv = meshVertexBuff[triIndex * 3 + 2].uv;
    vertexBuff[i * 3 + 2].size = meshVertexBuff[triIndex * 3 + 2].size * layerSize;
    vertexBuff[i * 3 + 2].color = multiplyColors(meshVertexBuff[triIndex * 3 + 2].color,  layerBuff[layerIndex].color);
    vertexBuff[i * 3 + 2].layer = layerIndex;

}