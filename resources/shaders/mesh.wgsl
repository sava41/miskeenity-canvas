
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

fn U32toVec2(a: u32)->vec2<u32> {
    return vec2(u32(( a >> 0 ) & 0xFFFF ), u32(( a >> 16 ) & 0xFFFF ));
}

@compute @workgroup_size(256, 1)
fn ma_main(@builtin(global_invocation_id) id_global : vec3<u32>, @builtin(local_invocation_id) id_local : vec3<u32>) {
    let i = u32(id_global.x);

    //Find which layer and which triangle within that layer's mesh we're processing
    var layerIndex =  u32(0);
    var remainingTris = i;

    while (layerIndex < uniforms.numLayers && remainingTris >= U32toVec2(layerBuff[layerIndex].meshOffsetLength).y) {
        remainingTris -= U32toVec2(layerBuff[layerIndex].meshOffsetLength).y;
        layerIndex++;
    }

    if (layerIndex >= uniforms.numLayers) {
        return;
    }

    let model = mat4x4<f32>(layerBuff[layerIndex].basisAX,  layerBuff[layerIndex].basisBX,  0.0, layerBuff[layerIndex].offsetX,
                            layerBuff[layerIndex].basisAY,  layerBuff[layerIndex].basisBY,  0.0, layerBuff[layerIndex].offsetY,
                            0.0,                            0.0,                            1.0, 0.0,
                            0.0,                            0.0,                            0.0, 1.0);

    let triIndex = U32toVec2(layerBuff[layerIndex].meshOffsetLength).x + remainingTris;

    vertexBuff[i * 3 + 0].xy = (vec4<f32>(meshVertexBuff[triIndex * 3 + 0].xy, 0.0, 1.0) * model).xy;
    vertexBuff[i * 3 + 0].uv = meshVertexBuff[triIndex * 3 + 0].uv;
    vertexBuff[i * 3 + 0].size = meshVertexBuff[triIndex * 3 + 0].size;
    vertexBuff[i * 3 + 0].color = meshVertexBuff[triIndex * 3 + 0].color;
    vertexBuff[i * 3 + 0].layer = layerIndex;

    vertexBuff[i * 3 + 1].xy = (vec4<f32>(meshVertexBuff[triIndex * 3 + 1].xy, 0.0, 1.0) * model).xy;
    vertexBuff[i * 3 + 1].uv = meshVertexBuff[triIndex * 3 + 1].uv;
    vertexBuff[i * 3 + 1].size = meshVertexBuff[triIndex * 3 + 1].size;
    vertexBuff[i * 3 + 1].color = meshVertexBuff[triIndex * 3 + 1].color;
    vertexBuff[i * 3 + 1].layer = layerIndex;

    vertexBuff[i * 3 + 2].xy = (vec4<f32>(meshVertexBuff[triIndex * 3 + 2].xy, 0.0, 1.0) * model).xy;
    vertexBuff[i * 3 + 2].uv = meshVertexBuff[triIndex * 3 + 2].uv;
    vertexBuff[i * 3 + 2].size = meshVertexBuff[triIndex * 3 + 2].size;
    vertexBuff[i * 3 + 2].color = meshVertexBuff[triIndex * 3 + 2].color;
    vertexBuff[i * 3 + 2].layer = layerIndex;

}