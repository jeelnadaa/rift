"""Local preview server for rift-static/ that behaves like Cloudflare Pages.

- Clean URLs: /download serves download.html; /download.html redirects to /download
- _redirects rules (simple "from to status" lines)
- Unknown paths get 404.html with status 404
- No caching, so CSS/JS edits show on reload

Usage: python scripts/serve-static.py [port]   (default 5174)
"""
import http.server, os, sys, urllib.parse

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'rift-static')
ROOT = os.path.normpath(ROOT)
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 5174


def load_redirects():
    rules = {}
    try:
        for line in open(os.path.join(ROOT, '_redirects'), encoding='utf8'):
            parts = line.split('#', 1)[0].split()
            if len(parts) >= 2:
                rules[parts[0]] = (parts[1], int(parts[2]) if len(parts) > 2 else 302)
    except FileNotFoundError:
        pass
    return rules


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=ROOT, **kw)

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def _redirect(self, to, code):
        self.send_response(code)
        self.send_header('Location', to)
        self.end_headers()

    def send_head(self):
        url = urllib.parse.urlsplit(self.path)
        path = urllib.parse.unquote(url.path)
        rules = load_redirects()
        if path in rules:
            to, code = rules[path]
            return self._redirect(to, code)
        # /page.html -> /page, /index.html -> /
        if path.endswith('.html') and path != '/404.html':
            clean = path[:-5]
            clean = clean[:-5] if clean.endswith('/index') else clean
            return self._redirect((clean or '/') + (('?' + url.query) if url.query else ''), 308)
        fs = os.path.join(ROOT, path.lstrip('/'))
        if os.path.isdir(fs):
            fs = os.path.join(fs, 'index.html')
        elif not os.path.exists(fs) and os.path.exists(fs + '.html'):
            fs = fs + '.html'
        if not os.path.isfile(fs) or not os.path.normpath(fs).startswith(ROOT):
            body = open(os.path.join(ROOT, '404.html'), 'rb').read()
            self.send_response(404)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body) if self.command != 'HEAD' else None
            return None
        self.path = '/' + os.path.relpath(fs, ROOT).replace(os.sep, '/')
        return super().send_head()


if __name__ == '__main__':
    print(f'Serving {ROOT} on http://127.0.0.1:{PORT}')
    http.server.ThreadingHTTPServer(('127.0.0.1', PORT), Handler).serve_forever()
