"""
Markdown 渲染服务 - 支持 Mermaid 图表和代码高亮
使用专用渲染线程解决 Playwright 跨线程问题
"""
import re
import json
import base64
import threading
from pathlib import Path
from typing import Optional
from queue import Queue

from markdown_it import MarkdownIt
from pygments import highlight
from pygments.lexers import get_lexer_by_name, TextLexer
from pygments.formatters import HtmlFormatter

from infrastructure.log_manager import get_logger


class QMLCodeFormatter(HtmlFormatter):
    """适配 QML RichText 的代码高亮格式化器"""
    def __init__(self, **options):
        super().__init__(**options)
        self.noclasses = True

    def wrap(self, source, outfile):
        yield 0, '<table width="100%" bgcolor="#1e1e1e" border="0" cellpadding="12"><tr><td><pre>'
        for i, t in source:
            yield i, t
        yield 0, '</pre></td></tr></table>'


class MermaidRenderThread(threading.Thread):
    """专用 Mermaid 渲染线程 - 持有 Playwright 实例"""

    def __init__(self):
        super().__init__(daemon=True)
        self._request_queue = Queue()
        self._running = True
        self._playwright = None
        self._browser = None
        self._page = None
        self.logger = get_logger("MermaidRenderThread")

    def run(self):
        """线程主循环"""
        while self._running:
            try:
                request = self._request_queue.get(timeout=1.0)
                if request is None:  # 停止信号
                    break
                code, result_event, result_holder = request
                try:
                    svg = self._render_mermaid(code)
                    result_holder['result'] = svg
                except Exception as e:
                    self.logger.error(f"Mermaid render error: {e}")
                    result_holder['result'] = None
                finally:
                    result_event.set()
            except:
                continue
        self._cleanup()

    def _get_page(self):
        """懒加载 Playwright 页面"""
        if self._page is None:
            try:
                from playwright.sync_api import sync_playwright
                self._playwright = sync_playwright().start()
                self._browser = self._playwright.chromium.launch(headless=True)
                self._page = self._browser.new_page()
                self.logger.info("Playwright initialized in render thread")
            except Exception as e:
                self.logger.error(f"Failed to init Playwright: {e}")
                return None
        return self._page

    def _render_mermaid(self, code: str) -> Optional[str]:
        """渲染 Mermaid 为 base64 SVG"""
        page = self._get_page()
        if not page:
            return None

        html_content = """<!DOCTYPE html><html><head><meta charset="utf-8">
<style>body{background:#09090b;font-family:"Microsoft YaHei",sans-serif;}</style>
</head><body><pre id="container" class="mermaid"></pre>
<script src="https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js"></script>
<script>
window.mermaidReady=false;
function waitForMermaid(){
    if(typeof mermaid!=='undefined'){
        mermaid.initialize({startOnLoad:false,theme:'dark',securityLevel:'loose',
            fontFamily:'"Microsoft YaHei",sans-serif',darkMode:true});
        window.mermaidReady=true;
    }else{setTimeout(waitForMermaid,100);}
}
waitForMermaid();
window.setMermaidCode=async function(code){
    try{document.getElementById('container').innerHTML=code;await mermaid.run();return{success:true};}
    catch(e){return{success:false,error:e.message};}
};
</script></body></html>"""

        page.set_content(html_content)
        page.wait_for_function("window.mermaidReady===true", timeout=10000)

        processed = code.replace("<", "&lt;").replace(">", "&gt;")
        processed = re.sub(r'&lt;br\s*/?&gt;', '<br/>', processed, flags=re.IGNORECASE)

        result = page.evaluate(f"window.setMermaidCode({json.dumps(processed)})")
        if not result.get('success', True):
            self.logger.error(f"Mermaid JS error: {result.get('error')}")
            return None

        page.wait_for_selector(".mermaid svg", state="visible", timeout=15000)
        page.wait_for_timeout(300)

        svg = page.locator(".mermaid svg").first.evaluate("el=>el.outerHTML")
        if not svg or len(svg) < 100:
            return None

        if 'xmlns="http://www.w3.org/2000/svg"' not in svg:
            svg = svg.replace('<svg', '<svg xmlns="http://www.w3.org/2000/svg"', 1)
        svg = svg.replace('&nbsp;', '&#160;')

        b64 = base64.b64encode(svg.encode('utf-8')).decode('utf-8')
        return f'<img src="data:image/svg+xml;base64,{b64}" width="100%" />'

    def render(self, code: str) -> Optional[str]:
        """线程安全的渲染请求"""
        result_event = threading.Event()
        result_holder = {'result': None}
        self._request_queue.put((code, result_event, result_holder))
        result_event.wait(timeout=30)
        return result_holder['result']

    def stop(self):
        """停止渲染线程"""
        self._running = False
        self._request_queue.put(None)

    def _cleanup(self):
        """清理资源"""
        if self._browser:
            self._browser.close()
        if self._playwright:
            self._playwright.stop()
        self.logger.info("MermaidRenderThread cleanup done")


class MarkdownService:
    """Markdown 渲染服务"""

    _instance = None
    _lock = threading.Lock()

    def __new__(cls):
        with cls._lock:
            if cls._instance is None:
                cls._instance = super().__new__(cls)
                cls._instance._initialized = False
            return cls._instance

    def __init__(self):
        if self._initialized:
            return
        self._initialized = True
        self.logger = get_logger("MarkdownService")

        self.md = MarkdownIt("commonmark", {"html": True, "linkify": True})
        self._setup_highlight()

        self._render_thread = MermaidRenderThread()
        self._render_thread.start()
        self.logger.info("MarkdownService initialized")

    def _setup_highlight(self):
        """配置代码高亮"""
        def highlight_code(code, lang, _attrs):
            try:
                lexer = get_lexer_by_name(lang) if lang else TextLexer()
            except:
                lexer = TextLexer()
            return highlight(code, lexer, QMLCodeFormatter(style="monokai", nowrap=True))
        self.md.options["highlight"] = highlight_code

    def _process_mermaid(self, markdown: str) -> str:
        """处理 Mermaid 代码块"""
        pattern = r'```mermaid\s*\n(.*?)\n```'

        def replace(match):
            code = match.group(1).strip()
            svg = self._render_thread.render(code)
            if svg:
                return f'\n{svg}\n'
            return f'<table width="100%" bgcolor="#27272a"><tr><td><pre style="color:#a1a1aa;">{code}</pre></td></tr></table>'

        return re.sub(pattern, replace, markdown, flags=re.DOTALL)

    def render(self, raw_md: str) -> str:
        """渲染 Markdown 为 HTML"""
        if not raw_md:
            return ""
        try:
            processed = self._process_mermaid(raw_md)
            body = self.md.render(processed)
            return f"{self._get_style()}{body}"
        except Exception as e:
            self.logger.error(f"Render failed: {e}")
            return f"<p style='color:#ef4444;'>渲染失败: {e}</p>"

    def _get_style(self) -> str:
        return """<style>
body{color:#e4e4e7;font-family:"Microsoft YaHei",sans-serif;font-size:14px;}
h1,h2,h3{color:#fff;margin-top:20px;margin-bottom:10px;font-weight:600;}
h1{font-size:24px;}h2{font-size:20px;}h3{font-size:16px;}
p{margin-bottom:12px;line-height:1.5;}
a{color:#60a5fa;}li{margin-bottom:6px;}
blockquote{color:#a1a1aa;font-style:italic;margin-left:20px;}
strong{font-weight:bold;color:#fff;}
code{font-family:monospace;color:#f472b6;}
</style>"""

    def shutdown(self):
        """关闭服务"""
        self._render_thread.stop()
        self._render_thread.join(timeout=5)
        self.logger.info("MarkdownService shutdown")
