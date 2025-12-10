"""
视频数据模型
定义视频相关的数据结构
"""
from dataclasses import dataclass
from typing import Optional
from datetime import datetime


@dataclass
class VideoModel:
    """视频数据模型"""
    
    path: str
    name: str
    duration: float = 0.0
    fps: float = 0.0
    width: int = 0
    height: int = 0
    size: int = 0
    created_at: Optional[datetime] = None
    
    @property
    def resolution(self) -> str:
        """获取分辨率字符串"""
        return f"{self.width}x{self.height}"
    
    @property
    def size_mb(self) -> float:
        """获取文件大小(MB)"""
        return self.size / (1024 * 1024)
    
    def is_valid(self) -> bool:
        """检查视频是否有效"""
        return (
            bool(self.path) and
            self.duration > 0 and
            self.width > 0 and
            self.height > 0
        )


@dataclass
class AnalysisResult:
    """分析结果模型"""
    
    video_path: str
    status: str  # "processing", "completed", "failed"
    progress: float = 0.0
    result_data: Optional[str] = None
    error_message: Optional[str] = None
    timestamp: Optional[datetime] = None
    
    def is_completed(self) -> bool:
        """是否完成"""
        return self.status == "completed"
    
    def is_failed(self) -> bool:
        """是否失败"""
        return self.status == "failed"
