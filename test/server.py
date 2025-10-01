# server.py
import http.server
import socketserver

PORT = 8000
DIRECTORY = "C:/Users/itsme/Downloads/MediaHuman/Music"

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

with socketserver.TCPServer(("", PORT), Handler) as httpd:
    print(f"HTTP server running on port {PORT}")
    print(f"Access files via: http://192.168.0.106:8000/LRchanal.mp3")
    httpd.serve_forever()