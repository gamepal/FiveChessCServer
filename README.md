# FiveChessCServer

CocosCreator1.5.2 VS2017

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### CocosCreator版本

game_client 开发连接测试
game_server 服务器

```
上传代码
git init
git add .
git commit -m 'commit'
git remote add origin https://github.com/game102/FiveChessCServer.git
git push -f origin master

拿代码
cd /opt/creator_server
git clone https://github.com/game102/FiveChessCServer.git
```

### 技术点
TCP Port
```
0号是保留端口
1-1024是固定端口：即被某些程序固定使用，一般不能使用。
1025-65535这些端口多数没有明确的定义服务对象，可以使用。
```
Select&IOCP
```
1:select模型句柄数量Win1024 Linux需要配置
2:IOCP与select的不同,select需要多次的重置管理句柄,IOCP只要一次;
3:有事件后select需要操作获取数据，而IOCP通知你的时候已经操作好了;
4:IOCP支持多线程同时等待;
```
长连接&短链接
```
长连接:初始化的时候，建立连接，一直不关闭;
          (1)服务器与客户端能以最快的速度发送数据;
          (2)服务器能主动的向客服端发送数据;
          (3):长链接长期占用网络资源;
          (4):长链接的服务器不方便直接重启;
短连接:发起连接--》发送数据--》获取数据-->关闭
          (1)不会长期占用资源;
          (2)方便服务器直接重启;
          (3):每次请求与获取数据都要从先建立连接;
          (4):服务器不能像客户端主动发送数据;
```
Websocket原理
```
1:解析客户端发送过来的报文;
GET /chat HTTP/1.1     (/char为url)
Host: server.example.com  (报文内容key:value start)
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==  (”来自客户端的随机”)
Origin: http://example.com
Sec-WebSocket-Protocol: chat, superchat
Sec-WebSocket-Version: 13   [报文内容key:value end]

2:返回key+migic ， SHA-1  加密， base-64 编码
key=”来自客户端的随机”, migic = “258EAFA5-E914-47DA-95CA-C5AB0DC85B11”;
HTTP/1.1 101 Switching Protocols
Upgrade: websocket   [报文内容key:value start]
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo= (”来自服务端编码”)
Sec-WebSocket-Protocol: chat  [报文内容key:value end]
```
### Unreal版本
A step by step series of examples that tell you how to get a development env running
Say what the step will be

```
Give the example
```
