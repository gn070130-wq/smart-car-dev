#!/usr/bin/env python3
"""UDP telemetry monitor and controller for ZouMaGuanBei.

Protocol: AA AA TYPE LENGTH PAYLOAD CHECKSUM, checksum is the 8-bit sum
of all bytes before CHECKSUM. Integers/floats are big-endian.
"""
import argparse, socket, struct, threading, time
import tkinter as tk
from tkinter import ttk, messagebox

HEAD = b'\xaa\xaa'; TELEMETRY = 2; COMMAND = 3

def checksum(data): return sum(data) & 0xff

def command(left, right, wp, wi, wd):
    payload = struct.pack('>iifff', int(left), int(right), float(wp), float(wi), float(wd))
    # Two signed speeds followed by three IEEE-754 float PID values.
    frame = HEAD + bytes((COMMAND, len(payload))) + payload
    return frame + bytes((checksum(frame),))

def parse(data):
    if len(data) != 41 or data[:2] != HEAD or data[2] != TELEMETRY or data[3] != 36 or checksum(data[:-1]) != data[-1]: return None
    vals = struct.unpack('>iiiii i fff', data[4:-1])
    return vals

class App:
    def __init__(self, root, bind, port, target, target_port):
        self.root=root; self.sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); self.sock.bind((bind,port)); self.target=(target,target_port)
        self.running=True; self.history=[[] for _ in range(5)]; self.t0=time.monotonic(); self.last=None
        root.title('ZouMaGuanBei UDP Monitor'); root.geometry('900x600'); root.protocol('WM_DELETE_WINDOW', self.close)
        top=ttk.Frame(root,padding=10); top.pack(fill='x'); self.vars=[]
        for name, val in [('left_expect_speed','0'),('right_expect_speed','0'),('wp','10'),('wi','2'),('wd','15')]:
            ttk.Label(top,text=name).pack(side='left',padx=(8,2)); v=tk.StringVar(value=val); self.vars.append(v); ttk.Entry(top,textvariable=v,width=9).pack(side='left')
        ttk.Button(top,text='发送参数',command=self.send).pack(side='left',padx=12); self.status=ttk.Label(root,text='等待遥测数据…'); self.status.pack(anchor='w',padx=12)
        self.canvas=tk.Canvas(root,bg='#101417',highlightthickness=0); self.canvas.pack(fill='both',expand=True,padx=12,pady=12); self.canvas.bind('<Configure>',lambda e:self.draw())
        threading.Thread(target=self.receive,daemon=True).start(); self.draw()
    def send(self):
        try: self.sock.sendto(command(*(v.get() for v in self.vars)),self.target); self.status.config(text='参数已发送')
        except (ValueError,OSError) as e: messagebox.showerror('发送失败',str(e))
    def receive(self):
        while self.running:
            try: data,_=self.sock.recvfrom(128); vals=parse(data)
            except OSError: break
            if vals:
                self.last=vals; now=time.monotonic()-self.t0
                for i,x in enumerate(vals[:5]): self.history[i].append((now,x)); self.history[i]=self.history[i][-240:]
                self.root.after(0,self.draw)
    def draw(self):
        c=self.canvas; c.delete('all'); w=max(c.winfo_width(),300); h=max(c.winfo_height(),200); colors=['#e85d5d','#62c6a8','#d9a441','#82a7d8','#c384c9']; names=['encoder L','encoder R','expect L','expect R','PWM L'];
        for i,(name,col) in enumerate(zip(names,colors)):
            y=(i+1)*h/6; c.create_line(45,y,w-15,y,fill='#29343a'); c.create_text(12,y,text=name,fill=col,anchor='w')
            pts=[]; hist=self.history[i];
            if hist:
                lo=min(v for _,v in hist); hi=max(v for _,v in hist); span=max(hi-lo,1)
                for t,v in hist: pts += [45+(t/max(time.monotonic()-self.t0,1e-3))*(w-60), y+25-(v-lo)/span*50]
                if len(pts)>3:c.create_line(*pts,fill=col,width=2)
        if self.last: self.status.config(text='L/R 编码器: %d / %d    L/R 目标: %d / %d    PWM: %d / %d    PID: %.3g / %.3g / %.3g'%self.last)
    def close(self): self.running=False; self.sock.close(); self.root.destroy()

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--bind',default='0.0.0.0'); p.add_argument('--port',type=int,default=9000); p.add_argument('--target',default='192.168.1.50'); p.add_argument('--target-port',type=int,default=9001); a=p.parse_args(); App(tk.Tk(),a.bind,a.port,a.target,a.target_port).root.mainloop()
