"""
Google Artifact to Markdown Converter (Mermaid图表专用版)
一键将Google Artifact文档转换为Markdown文件,自动将Mermaid图表转换为SVG图片

功能:
- 从Google Artifact页面提取完整的Markdown内容
- 检测所有Mermaid代码块
- 将Mermaid图表渲染为SVG图片
- 生成可直接导入Anytype的Markdown文件

使用方法:
    python artifact_to_markdown.py <artifact_url>
    
示例:
    python artifact_to_markdown.py https://g.co/gemini/share/xxxxx
"""

import sys
import os
import re
import time
import shutil
import json
from pathlib import Path
from datetime import datetime
from typing import Optional
from playwright.sync_api import sync_playwright, TimeoutError as PlaywrightTimeout
import argparse


class MermaidConverter:
    """Mermaid图表转换器"""
    
    def __init__(self, images_dir: Path, embed_base64: bool = False):
        self.images_dir = images_dir
        self.counter = 0
        self.embed_base64 = embed_base64
    
    def render_mermaid_to_svg(self, mermaid_code: str, page) -> Optional[str]:
        """
        将Mermaid代码渲染为SVG（仅使用本地Playwright渲染）
        """
        self.counter += 1
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        image_filename = f"mermaid_{self.counter}_{timestamp}.svg"
        image_path = self.images_dir / image_filename
        
        try:
            import base64
            
            print(f"  ... 正在本地渲染Mermaid图表 {self.counter}...")
            
            # 使用本地 Playwright 渲染
            # 使用 innerHTML 设置内容，以支持 <br/> 等 HTML 标签
            # 但需要对特定字符进行转义，避免破坏 HTML 结构
            html_content = """
            <!DOCTYPE html>
            <html>
            <head>
                <meta charset="utf-8">
                <style>
                    body { background-color: white; margin: 0; padding: 10px; }
                    /* 确保容器紧贴内容 */
                    #container { display: inline-block; }
                </style>
            </head>
            <body>
                <pre id="container" class="mermaid"></pre>
                <script src="https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js"></script>
                <script>
                    // 标记 Mermaid 是否准备就绪
                    window.mermaidReady = false;
                    window.renderError = null;
                    
                    // 等待 Mermaid 加载完成
                    function waitForMermaid() {
                        if (typeof mermaid !== 'undefined') {
                            mermaid.initialize({ 
                                startOnLoad: false, 
                                theme: 'default',
                                securityLevel: 'loose',
                                htmlLabels: true,
                            });
                            window.mermaidReady = true;
                        } else {
                            setTimeout(waitForMermaid, 100);
                        }
                    }
                    waitForMermaid();
                    
                    // 通过JS设置内容
                    window.setMermaidCode = async function(code) {
                        try {
                            // 使用 innerHTML 以支持 <br/> 等 HTML 标签
                            document.getElementById('container').innerHTML = code;
                            await mermaid.run();
                            return { success: true };
                        } catch (e) {
                            window.renderError = e.message;
                            return { success: false, error: e.message };
                        }
                    };
                </script>
            </body>
            </html>
            """
            
            page.set_content(html_content)
            
            # 等待 Mermaid 库加载完成
            try:
                page.wait_for_function("window.mermaidReady === true", timeout=10000)
            except Exception as e:
                print(f"  [Error] Mermaid库加载超时: {e}")
                return None
            
            # 对 Mermaid 代码进行预处理
            # 保留 <br/> 和 <br> 标签，但转义其他可能破坏 HTML 的字符
            # 必须转义 < 和 >，否则 classDiagram 的 <<interface>> 会被当做 HTML 标签
            processed_code = mermaid_code.replace("<", "&lt;").replace(">", "&gt;")
            # 恢复 <br/> 标签 (Mermaid 支持 <br/> 换行)
            processed_code = re.sub(r'&lt;br\s*/?&gt;', '<br/>', processed_code, flags=re.IGNORECASE)
            # 注意：Mermaid 语法中的 <<interface>>, <|--, --> 等需要保留
            # 它们不会破坏 HTML 结构，因为它们在引号内或作为连接符使用
            
            # 通过 JavaScript 注入 Mermaid 代码
            # 使用 JSON 序列化确保特殊字符正确传递
            escaped_for_js = json.dumps(processed_code)
            result = page.evaluate(f"window.setMermaidCode({escaped_for_js})")
            
            if result and not result.get('success', True):
                print(f"  [Error] Mermaid渲染错误: {result.get('error', 'Unknown error')}")
                return None
            
            # 等待SVG生成
            try:
                page.wait_for_selector(".mermaid svg", state="visible", timeout=15000)
                # 额外等待一小会儿确保渲染完全
                page.wait_for_timeout(500)
            except Exception as e:
                # 检查是否有渲染错误
                render_error = page.evaluate("window.renderError")
                if render_error:
                    print(f"  [Error] Mermaid渲染失败: {render_error}")
                else:
                    print(f"  [Error] 本地渲染超时: {e}")
                return None

            # 获取SVG内容
            try:
                svg_element = page.locator(".mermaid svg").first
                svg_content = svg_element.evaluate("el => el.outerHTML")
                
                # 验证SVG内容
                if not svg_content or len(svg_content) < 100:
                    print(f"  [Error] SVG内容无效或过短")
                    return None

                # 修复 SVG 兼容性问题 (Anytype 等需要标准头)
                # 1. 确保有 XML 声明
                if not svg_content.startswith('<?xml'):
                    svg_content = '<?xml version="1.0" encoding="UTF-8"?>\n' + svg_content
                
                # 2. 确保 svg 标签有 xmlns 属性
                if 'xmlns="http://www.w3.org/2000/svg"' not in svg_content:
                    svg_content = svg_content.replace('<svg', '<svg xmlns="http://www.w3.org/2000/svg"', 1)
                
                # 3. 替换 XML 不支持的 HTML 实体
                # Anytype/Strict XML parsers do not support &nbsp;
                svg_content = svg_content.replace('&nbsp;', '&#160;')
                
                # 4. 修复未闭合的 <br> 标签 (常见于 foreignObject 中)
                # 使用正则将 <br> (不带 /) 替换为 <br/>
                svg_content = re.sub(r'<br>(?!</br>)', '<br/>', svg_content)
                
                # 5. 移除非法字符或控制字符 (可选)
                
                if self.embed_base64:
                    base64_data = base64.b64encode(svg_content.encode('utf-8')).decode('utf-8')
                    return f"data:image/svg+xml;base64,{base64_data}"
                else:
                    with open(image_path, 'w', encoding='utf-8') as f:
                        f.write(svg_content)
                    print(f"  [OK] Mermaid图表 {self.counter}")
                    return f"images/{image_filename}"
            except Exception as e:
                print(f"  [Error] 获取SVG内容失败: {e}")
                return None
            
        except Exception as e:
            print(f"  [Error] Mermaid图表 {self.counter} 处理失败: {e}")
            return None


class ArtifactConverter:
    def __init__(self, url: str, output_dir: Optional[str] = None, embed_base64: bool = False):
        self.url = url
        if output_dir:
            self.output_dir = Path(output_dir)
        else:
            # 默认输出到桌面上的 Artifacts_Export 文件夹
            desktop = Path(os.path.join(os.path.expanduser("~"), "Desktop"))
            self.output_dir = desktop / "Artifacts_Export"
            
        self.images_dir = self.output_dir / "images"
        self.embed_base64 = embed_base64
        self.markdown_content = ""
        
    def setup_directories(self):
        """创建输出目录"""
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.images_dir.mkdir(parents=True, exist_ok=True)
        print(f"[OK] 输出目录: {self.output_dir}")
        print(f"[OK] 图片目录: {self.images_dir}")
        
    def extract_markdown_from_page(self, page):
        """从页面提取内容或直接读取本地文件"""
        
        # 检查是否为本地文件
        if os.path.exists(self.url):
            if os.path.isdir(self.url):
                print(f"[Error] 输入路径是一个目录: {self.url}")
                print(f"[Tip] 请提供具体的Markdown文件路径 (.md)")
                sys.exit(1)
                
            print(f"[File] 检测到本地文件，正在读取: {self.url}")
            with open(self.url, 'r', encoding='utf-8') as f:
                markdown_text = f.read()
            print(f"[OK] 成功读取本地内容 ({len(markdown_text)} 字符)")
            return markdown_text

        print(f"\n[Search] 开始从网页提取内容: {self.url}\n")
        try:
            # 访问页面
            print("... 正在加载页面 ...")
            page.goto(self.url, wait_until="networkidle", timeout=60000)
            page.wait_for_timeout(3000)  # 等待内容渲染
            
            # 尝试多种方式提取Markdown内容
            markdown_text = None
            
            # 方法1: 查找预格式化的文本内容
            pre_elements = page.query_selector_all('pre, code')
            for element in pre_elements:
                text = element.inner_text()
                if len(text) > 100 and ('```' in text or '#' in text):
                    markdown_text = text
                    break
            
            # 方法2: 直接获取主内容区域的文本
            if not markdown_text:
                content_selectors = [
                    'article',
                    '[role="article"]',
                    'main',
                    '.content',
                    '.markdown-body'
                ]
                
                for selector in content_selectors:
                    element = page.query_selector(selector)
                    if element:
                        markdown_text = element.inner_text()
                        if len(markdown_text) > 100:
                            print(f"[OK] 找到内容区域: {selector}")
                            break
            
            # 方法3: 使用JavaScript提取
            if not markdown_text:
                print("[!] 使用JavaScript提取内容...")
                markdown_text = page.evaluate("""
                    () => {
                        const article = document.querySelector('article') || 
                                      document.querySelector('[role="article"]') ||
                                      document.querySelector('main') ||
                                      document.body;
                        return article.innerText;
                    }
                """)
            
            if not markdown_text or len(markdown_text) < 50:
                raise Exception("无法提取到有效的Markdown内容")
            
            print(f"[OK] 成功提取内容 ({len(markdown_text)} 字符)")
            return markdown_text
            
        except Exception as e:
            print(f"[Error] 内容提取失败: {e}")
            raise
    
    def process_mermaid_blocks(self, markdown_text: str, page) -> str:
        """
        处理Markdown中的Mermaid代码块,将其转换为图片
        
        Args:
            markdown_text: 原始Markdown文本
            page: Playwright页面对象
            
        Returns:
            处理后的Markdown文本
        """
        print("\n[Process] 开始处理Mermaid图表...\n")
        
        # 正则表达式匹配Mermaid代码块
        mermaid_pattern = r'```mermaid\s*\n(.*?)\n```'
        
        mermaid_blocks = re.findall(mermaid_pattern, markdown_text, re.DOTALL)
        
        if not mermaid_blocks:
            print("[!] 未检测到Mermaid图表")
            return markdown_text
        
        print(f"[OK] 检测到 {len(mermaid_blocks)} 个Mermaid图表\n")
        
        # 创建Mermaid转换器
        converter = MermaidConverter(self.images_dir, self.embed_base64)
        
        # 处理每个Mermaid块
        def replace_mermaid(match):
            mermaid_code = match.group(1).strip()
            
            # 渲染为SVG
            image_path = converter.render_mermaid_to_svg(mermaid_code, page)
            
            if image_path:
                # 替换为图片引用
                return f"\n![Mermaid图表 {converter.counter}]({image_path})\n"
            else:
                # 保留原始代码块
                return match.group(0)
        
        # 替换所有Mermaid块
        processed_text = re.sub(mermaid_pattern, replace_mermaid, markdown_text, flags=re.DOTALL)
        
        print(f"\n[OK] 成功转换 {converter.counter} 个Mermaid图表")
        
        return processed_text
    
    def save_markdown(self, content: str, filename: Optional[str] = None):
        """保存Markdown文件"""
        if not filename:
            if os.path.exists(self.url):
                # 如果是本地文件,使用原文件名
                base_name = Path(self.url).stem
                filename = f"{base_name}_processed.md"
            else:
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"artifact_{timestamp}.md"
        
        output_file = self.output_dir / filename
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(content)
        
        print(f"\n[Done] Markdown文件已保存: {output_file}")
        print(f"[Done] 图片文件夹: {self.images_dir}")
        
        return output_file
    
    def create_zip(self):
        """将输出目录压缩为ZIP文件，并在完成后删除原目录"""
        if self.embed_base64:
            return None
            
        zip_name = self.output_dir.with_suffix('.zip')
        # 移除旧的zip
        if zip_name.exists():
            zip_name.unlink()
            
        print(f"\n[Process] 正在创建压缩包...")
        shutil.make_archive(str(self.output_dir), 'zip', self.output_dir)
        print(f"[OK] 压缩包已生成: {zip_name}")
        
        # 删除原文件夹
        try:
            shutil.rmtree(self.output_dir)
            print(f"[OK] 已清理临时文件夹")
        except Exception as e:
            print(f"[!] 清理文件夹失败: {e}")
            
        return zip_name

    def convert(self, output_filename: Optional[str] = None):
        """执行完整转换流程"""
        self.setup_directories()
        
        with sync_playwright() as p:
            # 启动浏览器
            print("\n[Start] 启动浏览器...")
            # 默认使用无头模式以提高兼容性
            browser = p.chromium.launch(headless=True)
            page = browser.new_page()
            
            try:
                # 1. 提取Markdown内容
                markdown_text = self.extract_markdown_from_page(page)
                
                # 2. 处理Mermaid图表
                processed_text = self.process_mermaid_blocks(markdown_text, page)
                
                # 3. 保存文件
                output_file = self.save_markdown(processed_text, output_filename)
                
                # 4. 如果不是Base64模式，创建ZIP包
                if not self.embed_base64:
                    self.create_zip()
                    print(f"\n[Tip] 建议直接将生成的 ZIP 文件导入到 Anytype 或 Notion 中。")
                else:
                    print(f"\n[Tip] 已使用 Base64 嵌入图片，您可以直接将单个 Markdown 文件导入。")
                
                return output_file
                
            except PlaywrightTimeout:
                print("[Error] 页面加载超时,请检查URL是否正确")
                sys.exit(1)
            except Exception as e:
                print(f"[Error] 转换失败: {e}")
                import traceback
                traceback.print_exc()
                sys.exit(1)
            finally:
                browser.close()


def main():
    parser = argparse.ArgumentParser(
        description='将Google Artifact文档转换为Markdown文件(Mermaid图表自动转SVG)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python artifact_to_markdown.py https://g.co/gemini/share/xxxxx
  python artifact_to_markdown.py https://g.co/gemini/share/xxxxx -o my_notes
  python artifact_to_markdown.py https://g.co/gemini/share/xxxxx -b
        """
    )
    
    parser.add_argument('url', help='Google Artifact页面URL 或 本地Markdown文件路径')
    parser.add_argument('-o', '--output-dir', help='输出目录(默认: 桌面/Artifacts_Export)')
    parser.add_argument('-f', '--filename', help='输出文件名(默认: artifact_<timestamp>.md)')
    parser.add_argument('-b', '--base64', action='store_true', help='将图片以Base64格式嵌入Markdown(实现单文件复制)')
    
    args = parser.parse_args()
    
    print("=" * 60)
    print("  Google Artifact to Markdown Converter")
    print("  (Mermaid图表专用版 - SVG)")
    print("=" * 60)
    
    converter = ArtifactConverter(args.url, args.output_dir, args.base64)
    converter.convert(args.filename)


if __name__ == "__main__":
    main()
