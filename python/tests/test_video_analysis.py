#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
独立的视频分析测试脚本 (优化版)
用于测试已有视频的 AI 分析功能，无需通过 UI 录制

使用方法:
    # 使用指定的 recording_id
    python python/tests/test_video_analysis.py --recording-id 6d95be20-5ebe-4f4c-91e4-b23e770ce5b8 --video "D:\\VideoTestpath\\recording_20260116_215200_keyframes.mp4"
    
    # 自动创建新的 recording_id
    python python/tests/test_video_analysis.py --video "D:\\VideoTestpath\\test.mp4"
    
    # 仅分析不保存到数据库
    python python/tests/test_video_analysis.py --video "D:\\Videos\\test.mp4" --no-save

参数:
    --recording-id  指定已存在的 recording_id（可选）
    --video         视频文件的绝对路径（必需）
    --no-save       仅分析，不保存结果到数据库
"""
import sys
import os
import argparse
import uuid
from datetime import datetime
from pathlib import Path

# 添加项目根目录到路径
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, project_root)

# 加载环境变量
from dotenv import load_dotenv
load_dotenv(os.path.join(os.path.dirname(project_root), ".env"))

from infrastructure.log_manager import get_logger
from services.gemini_service import GeminiService
from services.history_service import HistoryService
from AiService.prompt_builder import PromptBuilder
from database.models import Recording


def analyze_existing_video(video_path: str, recording_id: str = None, save_to_db: bool = True):
    """
    分析已有的视频文件
    
    Args:
        video_path: 视频文件的绝对路径
        recording_id: 指定的 recording_id（如果为 None 则创建新的）
        save_to_db: 是否保存分析结果到数据库
    """
    logger = get_logger("TestVideoAnalysis")
    
    # 验证文件存在
    if not os.path.exists(video_path):
        logger.error(f"视频文件不存在: {video_path}")
        return None
    
    file_size = os.path.getsize(video_path)
    logger.info(f"=" * 60)
    logger.info(f"准备分析视频: {video_path}")
    logger.info(f"文件大小: {file_size / (1024*1024):.2f} MB")
    logger.info(f"Recording ID: {recording_id or '(将自动创建)'}")
    logger.info(f"=" * 60)
    
    # 初始化服务
    try:
        gemini_service = GeminiService()
        history_service = HistoryService()
        prompt_builder = PromptBuilder()
        logger.info("✅ 服务初始化成功")
    except Exception as e:
        logger.error(f"❌ 服务初始化失败: {e}")
        import traceback
        traceback.print_exc()
        return None
    
    # 处理 recording_id
    now = datetime.now()
    
    if recording_id:
        # 检查指定的 recording_id 是否存在
        existing = history_service.recording_dao.get_by_id(recording_id)
        if not existing:
            logger.warning(f"指定的 recording_id 不存在，将创建新记录: {recording_id}")
            try:
                db_recording = Recording(
                    record_id=recording_id,
                    original_video_path=video_path,
                    title=Path(video_path).stem,
                    file_size_bytes=file_size,
                    created_at=now.isoformat(),
                    updated_at=now.isoformat(),
                    metadata={"source": "test_script", "status": "completed"}
                )
                history_service.recording_dao.create(db_recording)
                logger.info(f"✅ 创建录制记录: {recording_id}")
            except Exception as e:
                logger.error(f"❌ 创建录制记录失败: {e}")
                return None
        else:
            logger.info(f"✅ 使用已存在的录制记录: {recording_id}")
    else:
        # 创建新的 recording_id
        recording_id = str(uuid.uuid4())
        try:
            db_recording = Recording(
                record_id=recording_id,
                original_video_path=video_path,
                title=Path(video_path).stem,
                file_size_bytes=file_size,
                created_at=now.isoformat(),
                updated_at=now.isoformat(),
                metadata={"source": "test_script", "status": "completed"}
            )
            history_service.recording_dao.create(db_recording)
            logger.info(f"✅ 创建新录制记录: {recording_id}")
        except Exception as e:
            logger.warning(f"录制记录创建失败 (可能已存在): {e}")
    
    # 构建提示词 (参考 RecorderViewModel 的流程)
    video_context = {
        "duration": 0,  # 如果需要可以用 ffprobe 获取
        "file_size": file_size,
    }
    prompt = prompt_builder.build_prompt(
        scenario_category="general",
        video_context=video_context
    )
    logger.info(f"Prompt 长度: {len(prompt)} 字符")
    logger.info(f"Prompt 预览 (前 300 字符): {prompt[:300]}...")
    
    # 执行分析 (参考 AIAnalysisWorker 的流程)
    logger.info("开始 AI 分析...")
    start_time = datetime.now()
    
    result = gemini_service.analyze_video(video_path, prompt)
    
    elapsed = (datetime.now() - start_time).total_seconds()
    logger.info(f"分析完成，耗时: {elapsed:.2f} 秒")
    
    if not result:
        logger.error("❌ 分析失败，未返回结果")
        return None
    
    # 打印结果摘要
    print("\n" + "=" * 60)
    print("📊 分析结果摘要")
    print("=" * 60)
    
    if "summary_md" in result:
        print(f"\n📝 摘要:\n{result['summary_md']}")
    
    if "key_findings" in result and result["key_findings"]:
        print(f"\n🔍 关键发现 ({len(result['key_findings'])} 项):")
        for i, finding in enumerate(result["key_findings"][:5], 1):
            print(f"   {i}. [{finding.get('category', 'N/A')}] {finding.get('title', 'N/A')}")
            print(f"      {finding.get('content', '')[:80]}...")
    
    if "timestamp_events" in result and result["timestamp_events"]:
        print(f"\n⏱️ 时间戳事件 ({len(result['timestamp_events'])} 项):")
        for event in result["timestamp_events"][:5]:
            ts = event.get("timestamp_seconds", 0)
            mins, secs = int(ts) // 60, int(ts) % 60
            print(f"   [{mins:02d}:{secs:02d}] {event.get('title', 'N/A')}")
    
    if "video_analysis_md" in result:
        md_preview = result["video_analysis_md"][:1000]
        print(f"\n📄 主报告预览 (前 1000 字符):\n{'='*40}")
        print(md_preview)
        print(f"{'='*40}")
        print(f"... 共 {len(result['video_analysis_md'])} 字符")
    
    print("\n" + "=" * 60)
    
    # 保存到数据库 (参考 HistoryViewModel._on_ai_analysis_finished 的流程)
    if save_to_db:
        logger.info("保存分析结果到数据库...")
        try:
            # 使用 HistoryService.save_ai_analysis_result (与正式流程一致)
            success = history_service.save_ai_analysis_result(recording_id, result)
            if success:
                logger.info(f"✅ 分析结果已保存到数据库，recording_id: {recording_id}")
            else:
                logger.warning("⚠️ 分析结果保存失败")
        except Exception as e:
            logger.error(f"保存分析结果时出错: {e}")
            import traceback
            traceback.print_exc()
    
    return result


def main():
    parser = argparse.ArgumentParser(
        description="独立的视频分析测试脚本",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "--video", "-v",
        required=True,
        help="视频文件的绝对路径"
    )
    parser.add_argument(
        "--recording-id", "-r",
        default=None,
        help="指定已存在的 recording_id（可选，不指定则创建新的）"
    )
    parser.add_argument(
        "--no-save",
        action="store_true",
        help="仅分析，不保存结果到数据库"
    )
    
    args = parser.parse_args()
    
    result = analyze_existing_video(
        video_path=args.video,
        recording_id=args.recording_id,
        save_to_db=not args.no_save
    )
    
    if result:
        print("\n✅ 测试完成！")
        sys.exit(0)
    else:
        print("\n❌ 测试失败")
        sys.exit(1)


if __name__ == "__main__":
    main()
