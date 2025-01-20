
@group(0) @binding(0) var inputTexture: texture_2d<f32>;

@group(1) @binding(0) var outputTexture: texture_storage_2d<rgba8unorm,write>;
@compute @workgroup_size(8, 8)

fn pre_alpha(@builtin(global_invocation_id) id: vec3<u32>) {
    let input = textureLoad(inputTexture, vec2<u32>(id.x, id.y), 0);
    
    textureStore(outputTexture, id.xy, vec4<f32>(input.rgb * input.a, input.a));
}