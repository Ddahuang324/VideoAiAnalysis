import sys
import os
import json

# Add project root to path
project_root = r"d:\编程\项目\AiVideoAnalsysSystem\python"
sys.path.insert(0, project_root)

from AiService.response_parser import ResponseParser

def test_schema_fix():
    print("=== Testing Schema Fix ===")
    parser = ResponseParser()
    
    # This JSON structure previously failed due to "visual" event_type
    test_json = {
        "video_analysis_md": "# Test Report\n\n## 📋 Overview\nThis is a test.",
        "summary_md": "This is a test summary with more than ten characters.",
        "key_findings": [
            {
                "sequence_order": 0,
                "category": "visual",
                "title": "Visual Discovery",
                "content": "Found some visual elements.",
                "confidence_score": 90,
                "related_timestamps": [1.0]
            }
        ],
        "timestamp_events": [
            {
                "timestamp_seconds": 1.0,
                "event_type": "visual",
                "title": "Visual Event",
                "description": "A visual event occurred.",
                "importance_score": 5
            },
            {
                "timestamp_seconds": 2.0,
                "event_type": "technical",
                "title": "Technical Event",
                "description": "A technical event occurred.",
                "importance_score": 8
            }
        ],
        "analysis_metadata": [
            {"key": "test", "value": "val", "data_type": "string"}
        ]
    }
    
    result = parser.parse(json.dumps(test_json))
    if result:
        print("[PASS] Schema validation succeeded with 'visual' and 'technical' event types.")
        return True
    else:
        print("[FAIL] Schema validation failed.")
        return False

if __name__ == "__main__":
    if test_schema_fix():
        sys.exit(0)
    else:
        sys.exit(1)
