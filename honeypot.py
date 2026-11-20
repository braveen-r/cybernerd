import datetime
import json
import socket
import threading

HOST = "0.0.0.0"
LOG_FILE = "honeypot_logs.json"
log_lock = threading.Lock()

# Decoy services: {port: (service_name, initial_banner, rejection_response)}
DECOY_SERVICES = {
    2222: (
        "SSH",
        b"SSH-2.0-OpenSSH_8.9p1 Ubuntu-3ubuntu0.10\r\n",
        b"Permission denied (publickey,password).\r\n",
    ),
    2323: (
        "TELNET-IOT",
        b"Welcome to BusyBox v1.35.0 (CYBERNERD-GW)\r\nlogin: ",
        b"Login incorrect\r\n",
    ),
    8080: (
        "HTTP-ADMIN",
        b"",  # HTTP clients send their request first before the server replies
        b"HTTP/1.1 401 Unauthorized\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nAccess Denied",
    ),
}

SUSPICIOUS_KEYWORDS = ("admin", "root", "pass", "select", "union", "wget", "curl", "/bin/sh", "cmd")


def classify_payload(payload: str) -> str:
    lowered = payload.lower()
    if any(keyword in lowered for keyword in SUSPICIOUS_KEYWORDS):
        return "HIGH"
    return "LOW"


def record_event(event: dict) -> None:
    with log_lock:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(json.dumps(event) + "\n")


def handle_client(conn: socket.socket, addr: tuple[str, int], dst_port: int) -> None:
    ip, src_port = addr
    service_name, banner, response = DECOY_SERVICES[dst_port]
    timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat()

    payload_text = ""
    try:
        conn.settimeout(5.0)
        if banner:
            conn.sendall(banner)
        data = conn.recv(1024)
        if data:
            payload_text = data.decode("utf-8", errors="replace").strip()
        conn.sendall(response)
    except socket.timeout:
        payload_text = "<timeout / no payload>"
    except OSError as exc:
        payload_text = f"<socket error: {exc}>"
    finally:
        severity = classify_payload(payload_text)
        print(f"[{severity}] {timestamp} | {service_name} ({dst_port}) <- {ip}:{src_port} | Payload: {payload_text!r}")
        record_event({
            "timestamp": timestamp,
            "service": service_name,
            "severity": severity,
            "src_ip": ip,
            "src_port": src_port,
            "dst_port": dst_port,
            "payload": payload_text,
        })
        conn.close()


def listen_on_port(port: int) -> None:
    service_name = DECOY_SERVICES[port][0]
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, port))
        server.listen(20)
        print(f"[*] {service_name} decoy listening on {HOST}:{port}")

        while True:
            conn, addr = server.accept()
            threading.Thread(
                target=handle_client,
                args=(conn, addr, port),
                daemon=True,
            ).start()


def start_honeypot() -> None:
    print(f"[*] Starting CYBERNERD Multi-Port Honeypot (logging to {LOG_FILE})")
    threads = []
    for port in DECOY_SERVICES:
        t = threading.Thread(target=listen_on_port, args=(port,), daemon=True)
        t.start()
        threads.append(t)

    for t in threads:
        t.join()


if __name__ == "__main__":
    start_honeypot()