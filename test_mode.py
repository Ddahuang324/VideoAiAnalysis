import sys
import os

# Add the build directory to path to find the module
# Assuming the built DLL is in build/Release or build/Debug or similar
# For simplicity, we just try to import it.

try:
    import Video_Recording_Moudle as vrm
    print("Module loaded successfully")
    
    recorder = vrm.ScreenRecorder()
    print(f"Default mode: {recorder.recorder_mode}")
    
    recorder.recorder_mode = vrm.RecorderMode.SNAPSHOT
    print(f"Mode after setting: {recorder.recorder_mode}")
    
    if recorder.recorder_mode == vrm.RecorderMode.SNAPSHOT:
        print("Enum comparison works")
        
except ImportError as e:
    print(f"Import failed: {e}")
except Exception as e:
    print(f"An error occurred: {e}")
