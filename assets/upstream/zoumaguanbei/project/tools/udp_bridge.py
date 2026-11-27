#!/usr/bin/env python3
"""Bridge the robot UDP protocol to a browser WebSocket UI.

Run this file, then open http://127.0.0.1:8765/ in a browser.
"""
import argparse, base64, hashlib, json, socket, struct, threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HEAD = b'\xaa\xaa'; TELEMETRY = 2; COMMAND = 3
clients = {}; clients_lock = threading.Lock()

def checksum(data): return sum(data) & 255

def make_command(data):
    payload = struct.pack('>iifff', int(data['left_expect_speed']), int(data['right_expect_speed']),
                          float(data['wp']), float(data['wi']), float(data['wd']))
    frame = HEAD + bytes((COMMAND, len(payload))) + payload
    return frame + bytes((checksum(frame),))

def parse_telemetry(data):
    if len(data) != 41 or data[:2] != HEAD or data[2] != TELEMETRY or data[3] != 36 or checksum(data[:-1]) != data[-1]:
        return None
    values = struct.unpack('>iiiiiifff', data[4:-1])
    return dict(encoder_left=values[0], encoder_right=values[1], left_expect_speed=values[2],
                right_expect_speed=values[3], pwm_left=values[4], pwm_right=values[5],
                wp=values[6], wi=values[7], wd=values[8])

def ws_send(sock, text):
    payload = text.encode(); n = len(payload)
    if n < 126: header = bytes((0x81, n))
    elif n < 65536: header = bytes((0x81, 126)) + struct.pack('>H', n)
    else: header = bytes((0x81, 127)) + struct.pack('>Q', n)
    sock.sendall(header + payload)

def ws_read(sock):
    def read_n(n):
        result=b''
        while len(result)<n:
            part=sock.recv(n-len(result))
            if not part: raise ConnectionError()
            result += part
        return result
    first, second = read_n(2); length=second & 127
    if length == 126: length=struct.unpack('>H',read_n(2))[0]
    elif length == 127: length=struct.unpack('>Q',read_n(8))[0]
    masked=second & 128; mask=read_n(4) if masked else b''; payload=read_n(length)
    if masked: payload=bytes(v ^ mask[i%4] for i,v in enumerate(payload))
    if first & 15 == 8: raise ConnectionError()
    return payload.decode()

def send_json(sock, obj):
    message=json.dumps(obj,separators=(',',':'))
    with clients_lock: lock=clients.get(sock)
    if lock is None: return False
    try:
        with lock: ws_send(sock,message)
        return True
    except OSError:
        with clients_lock: clients.pop(sock,None)
        return False

def broadcast(obj):
    with clients_lock: targets=list(clients)
    for client in targets: send_json(client,obj)

class Handler(BaseHTTPRequestHandler):
    server_version='UDPBridge/1.0'
    def log_message(self,*args): pass
    def do_GET(self):
        if self.headers.get('Upgrade','').lower() == 'websocket': return self.websocket()
        page=(Path(__file__).with_name('udp_monitor.html')).read_bytes()
        self.send_response(200); self.send_header('Content-Type','text/html; charset=utf-8'); self.send_header('Content-Length',str(len(page))); self.end_headers(); self.wfile.write(page)
    def websocket(self):
        key=self.headers.get('Sec-WebSocket-Key','')
        accept=base64.b64encode(hashlib.sha1((key+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest()).decode()
        self.send_response(101,'Switching Protocols'); self.send_header('Upgrade','websocket'); self.send_header('Connection','Upgrade'); self.send_header('Sec-WebSocket-Accept',accept); self.end_headers()
        sock=self.connection
        with clients_lock: clients[sock]=threading.Lock()
        try:
            while True:
                try:
                    data=json.loads(ws_read(sock))
                    msg_type=data.get('type')
                    if msg_type == 'set_target':
                        host = str(data.get('host', '')).strip()
                        port = int(data.get('port', 0))
                        if not host or not 1 <= port <= 65535:
                            raise ValueError('IP 或端口格式无效')
                        socket.getaddrinfo(host, port, type=socket.SOCK_DGRAM)
                        self.server.robot=(host, port)
                        send_json(sock,{'type':'status','ok':True,'message':f'UDP 目标已切换为 {host}:{port}'})
                        print(f'UDP target changed to {host}:{port}')
                    elif msg_type == 'set_parameters':
                        target=self.server.robot
                        frame=make_command(data)
                        sent=self.server.udp.sendto(frame,target)
                        send_json(sock,{'type':'status','ok':sent==len(frame),'message':f'参数帧已发送：{sent} 字节 → {target[0]}:{target[1]}'})
                        print(f'parameter frame: {sent} bytes -> {target[0]}:{target[1]}')
                    else:
                        raise ValueError('未知的消息类型')
                except (ValueError, KeyError, TypeError, json.JSONDecodeError, OSError) as error:
                    send_json(sock,{'type':'status','ok':False,'message':f'操作失败：{error}'})
                    print(f'websocket command error: {error}')
        except ConnectionError:
            pass
        finally:
            with clients_lock: clients.pop(sock,None)

def udp_loop(server):
    while True:
        try: data,_=server.udp.recvfrom(256)
        except OSError: return
        telemetry=parse_telemetry(data)
        if telemetry: broadcast(telemetry)

if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--listen',default='127.0.0.1'); parser.add_argument('--port',type=int,default=8765); parser.add_argument('--robot',default='192.168.1.50'); parser.add_argument('--robot-port',type=int,default=9001); parser.add_argument('--telemetry-port',type=int,default=9000); args=parser.parse_args()
    http=ThreadingHTTPServer((args.listen,args.port),Handler); http.udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); http.udp.bind(('0.0.0.0',args.telemetry_port)); http.robot=(args.robot,args.robot_port)
    threading.Thread(target=udp_loop,args=(http,),daemon=True).start(); print(f'open http://{args.listen}:{args.port}/; robot={http.robot}')
    try: http.serve_forever()
    except KeyboardInterrupt: pass
