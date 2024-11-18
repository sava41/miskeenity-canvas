@group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(1) @binding(0) var inputMask: texture_2d<f32>;
@group(2) @binding(0) var outputTexture: texture_storage_2d<rgba8unorm,write>;
@compute @workgroup_size(8, 8)

fn mask_multipy(@builtin(global_invocation_id) id: vec3<u32>) {
    let input = textureLoad(inputTexture, vec2<u32>(id.x, id.y), 0);
    let mask = textureLoad(inputMask, vec2<u32>(id.x, id.y), 0);
    
    textureStore(outputTexture, id.xy, vec4<f32>(input.rgb * mask.r, input.a * mask.r));
}

@compute @workgroup_size(8, 8)
fn inv_mask_multipy(@builtin(global_invocation_id) id: vec3<u32>) {
    let input = textureLoad(inputTexture, vec2<u32>(id.x, id.y), 0);
    let mask = textureLoad(inputMask, vec2<u32>(id.x, id.y), 0);
    
    textureStore(outputTexture, id.xy, vec4<f32>(input.rgb * (1.0 - mask.r), input.a * (1.0 - mask.r)));
}