import sys
import os
import math

# Add build paths
sys.path.insert(0, os.path.abspath("d:/编程/项目/AiVideoAnalsysSystem/build/python"))
if hasattr(os, 'add_dll_directory'):
    os.add_dll_directory(os.path.abspath("d:/编程/项目/AiVideoAnalsysSystem/build/python"))
    os.add_dll_directory(os.path.abspath("d:/编程/项目/AiVideoAnalsysSystem/build/bin"))

try:
    import analyzer_module as ana
    
    scene_config = ana.SceneChangeDetectorConfig()
    scene_config.similarity_threshold = 0.85
    val = scene_config.similarity_threshold
    print(f"Set: 0.85")
    print(f"Got: {val}")
    print(f"Equal: {val == 0.85}")
    print(f"Difference: {abs(val - 0.85)}")
    print(f"Is close: {math.isclose(val, 0.85, rel_tol=1e-6)}")
except Exception as e:
    print(f"Error: {e}")
