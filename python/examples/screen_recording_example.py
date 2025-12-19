"""
屏幕录制服务使用示例
演示如何使用 ScreenRecorderService 进行屏幕录制
"""
import sys
import time
from pathlib import Path

# 添加项目路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from services.video_service import ScreenRecorderService, EncoderPreset, VideoCodec


def example_basic_recording():
    """示例1: 基本录制功能"""
    print("\n=== 示例1: 基本录制 ===")
    
    service = ScreenRecorderService()
    
    try:
        # 开始录制
        output_path = "output_basic.mp4"
        if service.start_recording(output_path):
            print(f"录制已开始，输出到: {output_path}")
            
            # 录制 5 秒
            for i in range(5):
                time.sleep(1)
                stats = service.get_stats()
                print(f"[{i+1}s] 帧数: {stats['frame_count']}, "
                      f"已编码: {stats['encoded_count']}, "
                      f"FPS: {stats['current_fps']:.1f}")
            
            # 停止录制
            service.stop_recording()
            print("录制已停止")
            
            # 显示最终统计
            final_stats = service.get_stats()
            print(f"\n最终统计:")
            print(f"  总帧数: {final_stats['frame_count']}")
            print(f"  已编码: {final_stats['encoded_count']}")
            print(f"  丢帧数: {final_stats['dropped_count']}")
            print(f"  文件大小: {final_stats['output_file_size'] / 1024:.2f} KB")
    
    except Exception as e:
        print(f"录制出错: {e}")


def example_with_callbacks():
    """示例2: 使用回调函数"""
    print("\n=== 示例2: 使用回调函数 ===")
    
    service = ScreenRecorderService()
    
    # 定义进度回调
    def on_progress(frames: int, size: int):
        print(f"[进度回调] 已编码 {frames} 帧, 文件大小 {size / 1024:.2f} KB")
    
    # 定义错误回调
    def on_error(error_message: str):
        print(f"[错误回调] {error_message}")
    
    # 设置回调
    service.set_progress_callback(on_progress)
    service.set_error_callback(on_error)
    
    try:
        output_path = "output_with_callbacks.mp4"
        if service.start_recording(output_path):
            print(f"录制已开始，输出到: {output_path}")
            
            # 录制 5 秒
            time.sleep(5)
            
            # 停止录制
            service.stop_recording()
            print("录制已停止")
    
    except Exception as e:
        print(f"录制出错: {e}")


def example_pause_resume():
    """示例3: 暂停和恢复录制"""
    print("\n=== 示例3: 暂停和恢复 ===")
    
    service = ScreenRecorderService()
    
    try:
        output_path = "output_pause_resume.mp4"
        if service.start_recording(output_path):
            print(f"录制已开始，输出到: {output_path}")
            
            # 录制 2 秒
            print("录制 2 秒...")
            time.sleep(2)
            
            # 暂停
            service.pause_recording()
            print("录制已暂停")
            time.sleep(2)
            
            # 恢复
            service.resume_recording()
            print("录制已恢复")
            time.sleep(2)
            
            # 停止
            service.stop_recording()
            print("录制已停止")
            
            # 显示统计
            stats = service.get_stats()
            print(f"\n统计: {stats}")
    
    except Exception as e:
        print(f"录制出错: {e}")


def example_with_context_manager():
    """示例4: 使用上下文管理器"""
    print("\n=== 示例4: 使用上下文管理器 ===")
    
    try:
        with ScreenRecorderService() as service:
            # 设置回调
            service.set_progress_callback(
                lambda f, s: print(f"  进度: {f} 帧, {s/1024:.1f} KB")
            )
            
            # 开始录制
            output_path = "output_context_manager.mp4"
            if service.start_recording(output_path):
                print(f"录制已开始，输出到: {output_path}")
                
                # 录制 3 秒
                time.sleep(3)
                
                # 自动停止（上下文管理器退出时）
                print("即将自动停止...")
        
        print("录制已自动停止（上下文管理器退出）")
    
    except Exception as e:
        print(f"录制出错: {e}")


def example_custom_encoder_config():
    """示例5: 自定义编码器配置"""
    print("\n=== 示例5: 自定义编码器配置 ===")
    
    service = ScreenRecorderService()
    
    # 创建自定义配置
    config = service.create_encoder_config(
        width=1280,
        height=720,
        fps=60,
        bitrate=8000000,  # 8 Mbps
        crf=20,  # 更高质量
        preset=EncoderPreset.FASTER,
        codec=VideoCodec.H264
    )
    
    print(f"编码器配置: {config}")
    print(f"  分辨率: {config.width}x{config.height}")
    print(f"  帧率: {config.fps}")
    print(f"  码率: {config.bitrate / 1000000:.1f} Mbps")
    print(f"  CRF: {config.crf}")
    print(f"  预设: {config.preset}")
    print(f"  编码器: {config.codec}")
    
    # 注意: 当前版本可能不支持动态设置配置
    # 这里仅展示如何创建配置对象


def example_high_quality_recording():
    """示例6: 高质量录制"""
    print("\n=== 示例6: 高质量录制 ===")
    
    service = ScreenRecorderService()
    
    try:
        output_path = "output_high_quality.mp4"
        
        print("使用高质量设置录制...")
        print("  分辨率: 1920x1080")
        print("  帧率: 60 fps")
        print("  编码器: H.264")
        print("  预设: medium")
        
        if service.start_recording(output_path):
            print(f"录制已开始，输出到: {output_path}")
            
            # 显示实时状态
            for i in range(5):
                time.sleep(1)
                stats = service.get_stats()
                print(f"[{i+1}s] "
                      f"帧数: {stats['frame_count']}, "
                      f"FPS: {stats['current_fps']:.1f}, "
                      f"丢帧: {stats['dropped_count']}")
            
            service.stop_recording()
            print("录制已停止")
    
    except Exception as e:
        print(f"录制出错: {e}")


def main():
    """运行所有示例"""
    print("=" * 60)
    print("屏幕录制服务示例")
    print("=" * 60)
    
    # 运行示例（根据需要注释/取消注释）
    example_basic_recording()
    # example_with_callbacks()
    # example_pause_resume()
    # example_with_context_manager()
    # example_custom_encoder_config()
    # example_high_quality_recording()
    
    print("\n" + "=" * 60)
    print("示例运行完成")
    print("=" * 60)


if __name__ == "__main__":
    main()
