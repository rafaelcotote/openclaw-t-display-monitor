#!/usr/bin/env python3
import json
import os
import shutil
import socket
import subprocess
import time
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HOST = os.environ.get("OPENCLAW_DISPLAY_HOST", "0.0.0.0")
PORT = int(os.environ.get("OPENCLAW_DISPLAY_PORT", "8765"))
OPENCLAW_BIN = os.environ.get("OPENCLAW_BIN", "openclaw")
CACHE_TTL = float(os.environ.get("OPENCLAW_DISPLAY_CACHE_TTL", "5"))

CACHE = {"ts": 0.0, "payload": None}


def run_json(cmd, timeout=20):
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or proc.stdout.strip() or f"command failed: {' '.join(cmd)}")
    return json.loads(proc.stdout)


def run_text(cmd, timeout=10):
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    if proc.returncode != 0:
        return ""
    return proc.stdout.strip()


def get_primary_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


def read_meminfo():
    info = {}
    with open("/proc/meminfo", "r", encoding="utf-8") as f:
        for line in f:
            key, value = line.split(":", 1)
            info[key] = int(value.strip().split()[0])
    total = info.get("MemTotal", 0) * 1024
    available = info.get("MemAvailable", 0) * 1024
    used = max(total - available, 0)
    percent = round((used / total) * 100, 1) if total else 0.0
    return {"total": total, "used": used, "available": available, "percent": percent}


def read_uptime_hours():
    with open("/proc/uptime", "r", encoding="utf-8") as f:
        seconds = float(f.read().split()[0])
    return seconds / 3600.0


def read_loadavg():
    with open("/proc/loadavg", "r", encoding="utf-8") as f:
        parts = f.read().split()
    return {"load1": float(parts[0]), "load5": float(parts[1]), "load15": float(parts[2])}


def docker_summary():
    names_raw = run_text(["docker", "ps", "--format", "{{.Names}}"])
    names = [line.strip() for line in names_raw.splitlines() if line.strip()]
    return {
        "running": len(names),
        "names": names[:8],
    }


def human_compact(n):
    if n is None:
        return "?"
    n = float(n)
    for suffix, div in (("M", 1_000_000), ("k", 1_000)):
        if abs(n) >= div:
            val = n / div
            return f"{val:.1f}{suffix}" if abs(val) < 100 else f"{val:.0f}{suffix}"
    return str(int(n))


def short_session_key(key):
    if not key:
        return "n/a"
    return key if len(key) <= 28 else key[:25] + "..."


def format_local_time_info(dt=None):
    dt = dt or datetime.now().astimezone()
    weekdays = ["segunda", "terça", "quarta", "quinta", "sexta", "sábado", "domingo"]
    months = ["jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez"]
    return {
        "iso": dt.isoformat(),
        "time": dt.strftime("%H:%M"),
        "seconds": dt.strftime("%H:%M:%S"),
        "date": dt.strftime("%d/%m/%Y"),
        "weekday": weekdays[dt.weekday()],
        "dateShort": f"{dt.day:02d} {months[dt.month - 1]}",
    }


def build_payload():
    status = run_json([OPENCLAW_BIN, "status", "--usage", "--json"], timeout=25)
    recent = (status.get("sessions") or {}).get("recent") or []
    current = recent[0] if recent else {}
    usage_providers = (status.get("usage") or {}).get("providers") or []
    usage = usage_providers[0] if usage_providers else {}
    windows = usage.get("windows") or []

    usage_by_label = {w.get("label"): w for w in windows}
    five = usage_by_label.get("5h", {})
    week = usage_by_label.get("Week", {})

    disk = shutil.disk_usage("/")
    mem = read_meminfo()
    load = read_loadavg()
    uptime_hours = read_uptime_hours()
    docker = docker_summary()
    ip = get_primary_ip()
    now_local = datetime.now().astimezone()
    time_info = format_local_time_info(now_local)

    total_tokens = current.get("totalTokens")
    input_tokens = current.get("inputTokens")
    output_tokens = current.get("outputTokens")
    cache_read = current.get("cacheRead")

    payload = {
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "time": time_info,
        "device": {
            "hostname": socket.gethostname(),
            "ip": ip,
            "localTime": time_info["time"],
        },
        "openclaw": {
            "version": status.get("runtimeVersion"),
            "channel": status.get("updateChannel"),
            "gatewayService": (status.get("gatewayService") or {}).get("runtimeShort"),
            "agents": (status.get("agents") or {}).get("agents") or [],
            "sessionsCount": (status.get("sessions") or {}).get("count"),
            "defaultModel": ((status.get("sessions") or {}).get("defaults") or {}).get("model"),
            "currentSession": {
                "key": current.get("key"),
                "keyShort": short_session_key(current.get("key")),
                "model": current.get("model"),
                "totalTokens": total_tokens,
                "inputTokens": input_tokens,
                "outputTokens": output_tokens,
                "cacheRead": cache_read,
                "remainingTokens": current.get("remainingTokens"),
                "percentUsed": current.get("percentUsed"),
                "contextTokens": current.get("contextTokens"),
                "ageMs": current.get("age"),
                "totalTokensCompact": human_compact(total_tokens),
                "inputTokensCompact": human_compact(input_tokens),
                "outputTokensCompact": human_compact(output_tokens),
                "cacheReadCompact": human_compact(cache_read),
            },
            "providerUsage": {
                "provider": usage.get("provider"),
                "displayName": usage.get("displayName"),
                "plan": usage.get("plan"),
                "fiveHourUsedPercent": five.get("usedPercent"),
                "fiveHourLeftPercent": None if five.get("usedPercent") is None else 100 - five.get("usedPercent"),
                "weekUsedPercent": week.get("usedPercent"),
                "weekLeftPercent": None if week.get("usedPercent") is None else 100 - week.get("usedPercent"),
                "fiveHourResetAt": five.get("resetAt"),
                "weekResetAt": week.get("resetAt"),
            },
            "security": (status.get("securityAudit") or {}).get("summary") or {},
        },
        "system": {
            "uptimeHours": round(uptime_hours, 1),
            "load1": load["load1"],
            "load5": load["load5"],
            "load15": load["load15"],
            "memory": mem,
            "disk": {
                "total": disk.total,
                "used": disk.used,
                "free": disk.free,
                "percent": round((disk.used / disk.total) * 100, 1) if disk.total else 0.0,
            },
        },
        "docker": docker,
        "dashboard": {
            "statusText": "OpenClaw online" if (status.get("gatewayService") or {}).get("runtimeShort") else "OpenClaw status unknown",
            "lastUpdate": time_info["seconds"],
            "headline": f"{time_info['weekday']}, {time_info['dateShort']}",
        },
    }
    return payload


HTML = """<!doctype html>
<html>
<head>
  <meta charset=\"utf-8\" />
  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\" />
  <title>OpenClaw Display API</title>
  <style>
    body{font-family:system-ui,Arial,sans-serif;background:#0b1020;color:#e7ecff;margin:0;padding:24px}
    .card{max-width:860px;margin:auto;background:#131a33;border:1px solid #273156;border-radius:16px;padding:20px}
    code,pre{background:#0d1328;color:#9ee7ff;padding:2px 6px;border-radius:6px}
    pre{padding:16px;overflow:auto}
    a{color:#77d9ff}
  </style>
</head>
<body>
  <div class=\"card\">
    <h1>OpenClaw Display API</h1>
    <p>Use <code>/status</code> for the JSON payload consumed by the ESP32 dashboard.</p>
    <p>Example: <a href=\"/status\">/status</a></p>
  </div>
</body>
</html>"""


class Handler(BaseHTTPRequestHandler):
    def _send_json(self, obj, code=200):
        data = json.dumps(obj, ensure_ascii=False, indent=2).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def _send_html(self, html, code=200):
        data = html.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            self._send_html(HTML)
            return
        if self.path == "/preview":
            try:
                with open(Path(__file__).with_name("web-preview.html"), "r", encoding="utf-8") as f:
                    self._send_html(f.read())
            except Exception as e:
                self._send_json({"ok": False, "error": str(e)}, 500)
            return
        if self.path == "/openclaw-logo.svg":
            try:
                p = Path(__file__).with_name("openclaw-logo.svg")
                data = p.read_bytes()
                self.send_response(200)
                self.send_header("Content-Type", "image/svg+xml")
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)
            except Exception as e:
                self._send_json({"ok": False, "error": str(e)}, 500)
            return
        if self.path == "/status":
            try:
                now = time.time()
                if CACHE["payload"] is None or (now - CACHE["ts"]) > CACHE_TTL:
                    CACHE["payload"] = build_payload()
                    CACHE["ts"] = now
                self._send_json(CACHE["payload"])
            except Exception as e:
                self._send_json({"ok": False, "error": str(e)}, 500)
            return
        self._send_json({"ok": False, "error": "not found"}, 404)

    def log_message(self, fmt, *args):
        return


def main():
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"OpenClaw Display API listening on http://{HOST}:{PORT}")
    print(f"Try: http://{get_primary_ip()}:{PORT}/status")
    server.serve_forever()


if __name__ == "__main__":
    main()
