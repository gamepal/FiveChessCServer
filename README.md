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

### 技术点梳理
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
### Unreal版本
A step by step series of examples that tell you how to get a development env running
Say what the step will be

```
Give the example
```
