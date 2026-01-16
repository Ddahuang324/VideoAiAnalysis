"""AiService 集成测试"""
import sys
import os
import tempfile

# 添加项目根目录到路径
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, project_root)

# 加载 .env
from dotenv import load_dotenv
load_dotenv(os.path.join(os.path.dirname(project_root), ".env"))

# 直接导入模块，避免 __init__.py 的相对导入问题
import importlib.util
def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

response_parser = load_module("response_parser", os.path.join(project_root, "AiService", "response_parser.py"))
video_preprocessor = load_module("video_preprocessor", os.path.join(project_root, "AiService", "video_preprocessor.py"))
prompt_builder = load_module("prompt_builder", os.path.join(project_root, "AiService", "prompt_builder.py"))

ResponseParser = response_parser.ResponseParser
VideoPreprocessor = video_preprocessor.VideoPreprocessor
PromptBuilder = prompt_builder.PromptBuilder


def test_response_parser():
    """测试 ResponseParser"""
    print("\n=== 测试 ResponseParser ===")
    parser = ResponseParser()

    # 测试有效 JSON
    valid_json = '''{
        "summary": "这是一个测试视频的摘要内容",
        "key_findings": [
            {"category": "测试", "title": "发现1", "content": "内容", "confidence": 85}
        ],
        "timestamp_events": [
            {"timestamp": 0.0, "event_type": "highlight", "title": "开始"}
        ]
    }'''
    result = parser.parse(valid_json)
    assert result is not None, "有效 JSON 解析失败"
    print("[PASS] 有效 JSON 解析成功")

    # 测试 Markdown 包裹的 JSON
    markdown_json = f'```json\n{valid_json}\n```'
    result = parser.parse(markdown_json)
    assert result is not None, "Markdown JSON 解析失败"
    print("[PASS] Markdown JSON 解析成功")

    # 测试无效 JSON
    result = parser.parse("invalid json")
    assert result is None, "无效 JSON 应返回 None"
    print("[PASS] 无效 JSON 正确返回 None")


def test_video_preprocessor():
    """测试 VideoPreprocessor"""
    print("\n=== 测试 VideoPreprocessor ===")
    preprocessor = VideoPreprocessor()

    # 测试不存在的文件
    assert not preprocessor.validate_video("nonexistent.mp4"), "不存在文件应返回 False"
    print("[PASS] 不存在文件验证正确")

    # 测试不支持的格式
    with tempfile.NamedTemporaryFile(suffix=".txt", delete=False) as f:
        f.write(b"test")
        temp_path = f.name
    try:
        assert not preprocessor.validate_video(temp_path), "不支持格式应返回 False"
        print("[PASS] 不支持格式验证正确")
    finally:
        os.unlink(temp_path)

    # 测试有效视频文件（模拟）
    with tempfile.NamedTemporaryFile(suffix=".mp4", delete=False) as f:
        f.write(b"fake video content")
        temp_path = f.name
    try:
        assert preprocessor.validate_video(temp_path), "有效格式应返回 True"
        print("[PASS] 有效格式验证正确")

        # 测试元数据获取
        metadata = preprocessor.get_video_metadata(temp_path)
        assert "file_size" in metadata, "元数据应包含 file_size"
        print("[PASS] 元数据获取正确")
    finally:
        os.unlink(temp_path)


def test_prompt_builder():
    """测试 PromptBuilder"""
    print("\n=== 测试 PromptBuilder ===")
    builder = PromptBuilder()

    # 测试基本 prompt 构建
    prompt = builder.build_prompt()
    assert "视频内容分析助手" in prompt, "应包含系统提示词"
    assert "JSON格式" in prompt, "应包含输出格式"
    print("[PASS] 基本 prompt 构建成功")

    # 测试带上下文的 prompt
    context = {"duration": 120.5, "file_size": 1024 * 1024 * 10}
    prompt = builder.build_prompt(video_context=context)
    assert "120.5" in prompt, "应包含时长信息"
    print("[PASS] 带上下文 prompt 构建成功")


def test_gemini_api():
    """测试 Gemini API 连接"""
    print("\n=== 测试 Gemini API ===")
    from google import genai

    api_key = os.getenv("GOOGLE_API_KEY")
    if not api_key:
        print("[SKIP] 未设置 GOOGLE_API_KEY")
        return

    try:
        client = genai.Client(api_key=api_key)
        response = client.models.generate_content(
            model="gemini-2.5-flash-lite",
            contents="Reply OK"
        )
        assert response.text, "API 应返回响应"
        print(f"[PASS] Gemini API 响应正常: {response.text.strip()[:50]}")
    except Exception as e:
        if "429" in str(e) or "quota" in str(e).lower():
            print("[WARN] Gemini API 配额已用尽，API 连接正常但无法测试")
        else:
            raise


def main():
    print("=" * 50)
    print("AiService 集成测试")
    print("=" * 50)

    try:
        test_response_parser()
        test_video_preprocessor()
        test_prompt_builder()
        test_gemini_api()
        print("\n" + "=" * 50)
        print("所有测试通过!")
        print("=" * 50)
        return 0
    except AssertionError as e:
        print(f"\n测试失败: {e}")
        return 1
    except Exception as e:
        print(f"\n测试异常: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
