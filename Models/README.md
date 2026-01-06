# AI 模型下载指南

本文档提供项目所需的四个 AI 模型的下载链接和配置说明。

## 📦 模型清单

| 模型 | 用途 | 格式 | 文件大小 | 输入尺寸 |
|:----:|:----:|:----:|:--------:|:--------:|
| MobileNetV3-Small | 场景变化检测 | ONNX | ~2.5 MB | 224×224 |
| YOLOv8n | 运动检测 | ONNX | ~6.3 MB | 640×640 |
| PP-OCRv4 Det | 文字检测 | ONNX | 4.75 MB | 动态 |
| PP-OCRv4 Rec | 文字识别 | ONNX | 10.9 MB | 动态 |

**总大小**: ~24 MB

---

## 1️⃣ MobileNetV3-Small (场景变化检测)

### 下载方式

**方式一: 使用 PyTorch Hub 导出**

```python
import torch
import torchvision

# 加载预训练模型
model = torchvision.models.mobilenet_v3_small(pretrained=True)
model.eval()

# 导出为 ONNX
dummy_input = torch.randn(1, 3, 224, 224)
torch.onnx.export(
    model,
    dummy_input,
    "mobilenetv3_small.onnx",
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={'input': {0: 'batch_size'}, 'output': {0: 'batch_size'}}
)
```

**方式二: 直接下载 (推荐)**

```bash
# 从 Hugging Face 下载
wget https://huggingface.co/onnx-community/mobilenetv3-small-100/resolve/main/model.onnx -O mobilenetv3_small.onnx
```

### 模型规格

- **输入**: `[1, 3, 224, 224]` (NCHW, RGB, 归一化到 [0, 1])
- **输出**: `[1, 1280]` (特征向量)
- **预处理**: Resize → 归一化 → HWC→NCHW

---

## 2️⃣ YOLOv8n (运动检测)

### 下载方式

**步骤 1: 安装 Ultralytics**

```bash
pip install ultralytics
```

**步骤 2: 下载并导出**

```python
from ultralytics import YOLO

# 下载预训练模型
model = YOLO('yolov8n.pt')

# 导出为 ONNX
model.export(format='onnx')
```

或使用命令行:

```bash
yolo export model=yolov8n.pt format=onnx
```

**直接下载 PT 模型**:
```bash
wget https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n.pt
```

### 模型规格

- **输入**: `[1, 3, 640, 640]` (NCHW, RGB, 归一化到 [0, 1])
- **输出**: 
  - `output0`: `[1, 84, 8400]` (检测框 + 类别概率)
- **预处理**: Resize(保持比例) → Padding → 归一化

---

## 3️⃣ PP-OCRv4 检测模型 (文字检测)

### 下载方式

**直接下载 ONNX 模型 (推荐)**

```bash
# 从 RapidOCR Hugging Face 仓库下载
wget https://huggingface.co/SWHL/RapidOCR/resolve/main/PP-OCRv4/ch_PP-OCRv4_det_infer.onnx
```

或使用浏览器访问:
- [ch_PP-OCRv4_det_infer.onnx](https://huggingface.co/SWHL/RapidOCR/resolve/main/PP-OCRv4/ch_PP-OCRv4_det_infer.onnx)

### 模型规格

- **输入**: `[1, 3, H, W]` (动态尺寸, NCHW, RGB)
- **输出**: 文本区域检测框坐标
- **预处理**: Resize(限制最大边) → 归一化 → HWC→NCHW

---

## 4️⃣ PP-OCRv4 识别模型 (文字识别)

### 下载方式

**直接下载 ONNX 模型 (推荐)**

```bash
# 从 RapidOCR Hugging Face 仓库下载
wget https://huggingface.co/SWHL/RapidOCR/resolve/main/PP-OCRv4/ch_PP-OCRv4_rec_infer.onnx
```

或使用浏览器访问:
- [ch_PP-OCRv4_rec_infer.onnx](https://huggingface.co/SWHL/RapidOCR/resolve/main/PP-OCRv4/ch_PP-OCRv4_rec_infer.onnx)

### 模型规格

- **输入**: `[1, 3, 48, W]` (动态宽度, NCHW, RGB)
- **输出**: 文本识别结果(字符序列)
- **预处理**: Resize(固定高度48) → 归一化 → HWC→NCHW

---

## 📂 目录结构

下载完成后,将模型文件放置在以下位置:

```
AiVideoAnalsysSystem/
└── models/
    ├── mobilenetv3_small.onnx          # 2.5 MB
    ├── yolov8n.onnx                    # 6.3 MB
    ├── ch_PP-OCRv4_det_infer.onnx      # 4.75 MB
    └── ch_PP-OCRv4_rec_infer.onnx      # 10.9 MB
```

---

## ✅ 验证模型完整性

### 使用 Python 验证

```python
import onnx

models = [
    "mobilenetv3_small.onnx",
    "yolov8n.onnx",
    "ch_PP-OCRv4_det_infer.onnx",
    "ch_PP-OCRv4_rec_infer.onnx"
]

for model_path in models:
    try:
        model = onnx.load(model_path)
        onnx.checker.check_model(model)
        print(f"✓ {model_path} - 验证通过")
    except Exception as e:
        print(f"✗ {model_path} - 验证失败: {e}")
```

### 预期文件大小

| 文件名 | 预期大小 |
|:------:|:--------:|
| `mobilenetv3_small.onnx` | ~2.5 MB |
| `yolov8n.onnx` | ~6.3 MB |
| `ch_PP-OCRv4_det_infer.onnx` | 4.75 MB |
| `ch_PP-OCRv4_rec_infer.onnx` | 10.9 MB |

---

## 🚀 性能说明

### RapidOCR ONNX 优势

相比原生 PaddlePaddle 推理:
- ✅ **速度提升 4-5 倍**
- ✅ **跨平台部署** (Windows/Linux/macOS)
- ✅ **无需 PaddlePaddle 依赖**
- ✅ **支持 CPU/GPU 加速**

### 推理性能预估 (CPU - 4核)

| 模型 | 推理时间 |
|:----:|:--------:|
| MobileNetV3-Small | 5-10ms |
| YOLOv8n | 15-30ms |
| PP-OCRv4 Det | 20-40ms |
| PP-OCRv4 Rec | 10-20ms |

---

## 📚 参考资源

- [ONNX Runtime 官方文档](https://onnxruntime.ai/)
- [RapidOCR GitHub](https://github.com/RapidAI/RapidOCR)
- [Ultralytics YOLOv8](https://github.com/ultralytics/ultralytics)
- [MobileNetV3 论文](https://arxiv.org/abs/1905.02244)
- [PP-OCRv4 技术报告](https://arxiv.org/abs/2303.18248)
