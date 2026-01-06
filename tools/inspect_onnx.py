import onnxruntime as ort
import numpy as np

def inspect_model(model_path):
    try:
        session = ort.InferenceSession(model_path)
        print(f"Model: {model_path}")
        
        print("\nInputs:")
        for input in session.get_inputs():
            print(f"  Name: {input.name}, Shape: {input.shape}, Type: {input.type}")
            
        print("\nOutputs:")
        for output in session.get_outputs():
            print(f"  Name: {output.name}, Shape: {output.shape}, Type: {output.type}")
            
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        inspect_model(sys.argv[1])
    else:
        inspect_model("Models/MobileNet-v3-Small.onnx")
