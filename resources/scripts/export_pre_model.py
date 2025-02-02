# Script to convert pre-process same model to onnx 
# based on script from dinglufe https://github.com/dinglufe/segment-anything-cpp-wrapper/blob/main/export_pre_model.py

import torch
import numpy as np
import os
from pathlib import Path

def create_sam_preprocessor(model_config):
    """
    Initialize SAM model based on configuration.
    """
    if model_config['type'] == 'mobile':
        import mobile_sam as SAM
    else:
        import segment_anything as SAM
    
    from segment_anything.utils.transforms import ResizeLongestSide
    
    sam = SAM.sam_model_registry[model_config['model_type']](checkpoint=model_config['checkpoint'])
    sam.to(device='cpu')
    transform = ResizeLongestSide(sam.image_encoder.img_size)
    
    return sam, transform, SAM

class PreprocessModel(torch.nn.Module):
    def __init__(self, image_size, sam, SAM):
        super().__init__()
        self.sam = sam
        self.predictor = SAM.SamPredictor(self.sam)
        self.image_size = image_size
        
    def forward(self, x):
        self.predictor.set_torch_image(x, self.image_size)
        if not hasattr(self.predictor, 'interm_features'):
            return self.predictor.get_image_embedding()
        else:
            return (self.predictor.get_image_embedding(), 
                   torch.stack(self.predictor.interm_features, dim=0))

def export_model(config):
    """
    Export SAM preprocessing model to ONNX format.
    """
    # Create output directories
    output_dir = Path(config['output_path']).parent
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if config['quantize']:
        output_name = Path(config['output_path']).stem
        raw_dir = output_dir / f"{output_name}_raw"
        raw_dir.mkdir(exist_ok=True)
        output_raw_path = raw_dir / f"{output_name}.onnx"
    else:
        output_raw_path = config['output_path']

    # Initialize model
    sam, transform, SAM = create_sam_preprocessor(config)
    
    # Prepare input
    image = np.zeros((config['image_size'][1], config['image_size'][0], 3), 
                     dtype=np.uint8)
    input_image = transform.apply_image(image)
    input_tensor = torch.as_tensor(input_image, device='cpu')
    input_tensor = input_tensor.permute(2, 0, 1).contiguous()[None, :, :, :]

    # Create and export model
    model = PreprocessModel(config['image_size'], sam, SAM)
    model.eval()
    
    with torch.no_grad():
        model_trace = torch.jit.trace(model, input_tensor)
        torch.onnx.export(
            model_trace,
            input_tensor,
            str(output_raw_path),
            input_names=['input'],
            output_names=config['output_names'],
            opset_version=14
        )

    # Quantize if requested
    if config['quantize']:
        from onnxruntime.quantization import QuantType
        from onnxruntime.quantization.quantize import quantize_dynamic
        
        quantize_dynamic(
            model_input=str(output_raw_path),
            model_output=config['output_path'],
            per_channel=False,
            reduce_range=False,
            weight_type=QuantType.QUInt8
        )

if __name__ == "__main__":
    # Configuration for different models
    configs = {
        'sam': {
            'type': 'sam',
            'model_type': 'vit_h',
            'checkpoint': 'sam_vit_h_4b8939.pth',
            'output_path': 'models/sam_preprocess.onnx',
            'quantize': True,
            'output_names': ['output'],
            'image_size': (1024, 1024)
        },
        'mobile_sam': {
            'type': 'mobile',
            'model_type': 'vit_t',
            'checkpoint': 'mobile_sam.pt',
            'output_path': 'models/mobile_sam_preprocess.onnx',
            'quantize': False,
            'output_names': ['output'],
            'image_size': (1024, 1024)
        },
        'sam_hq': {
            'type': 'sam',
            'model_type': 'vit_h',
            'checkpoint': 'sam_hq_vit_h.pth',
            'output_path': 'models/sam_hq_preprocess.onnx',
            'quantize': True,
            'output_names': ['output', 'interm_embeddings'],
            'image_size': (1024, 1024)
        }
    }
    
    # Select which model to export
    model_type = 'sam'  # Change this to 'sam' or 'mobile_sam' as needed
    export_model(configs[model_type])