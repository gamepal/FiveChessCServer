var websocket = {    //作为一个module导出
    sock: null, 
    cmd_handler: null,   //收到服务类型，其对应的响应函数function(opt_cmd)。 由函数register_cmd_handler来注册
    is_connected: false,
    
    _on_opened: function(event) {   //下划线开头为我们自己定义
        console.log("ws connect server success");
        this.is_connected = true;
    }, 
    
    _on_recv_data: function(event) {
        var cmd_json = event.data;

        if (!cmd_json || !this.cmd_handler) {
            return;
        }
        
        var cmd = JSON.parse(cmd_json);
        if (!cmd) {
            return;
        }
        
        var stype = cmd[0]; // 服务类型
        if (this.cmd_handler[stype]) {
            this.cmd_handler[stype](cmd);  //访问服务类型的回调函数
        }
    },

    _on_socket_close: function(event) {
        this.is_connected = false;
        if (this.sock) {
            this.close();
        }
    },   
    
    _on_socket_err: function(event) {
        this._on_socket_close();
    }, 

    connect: function(url) {
        this.sock = new WebSocket(url);
        
        this.sock.onopen = this._on_opened.bind(this);
        this.sock.onmessage = this._on_recv_data.bind(this);
        this.sock.onclose = this._on_socket_close.bind(this);
        this.sock.onerror = this._on_socket_err.bind(this);
    },   
    


    send: function(body) {
        if (this.is_connected && this.sock) {
            this.sock.send(body);
        }
    }, 

    send_object: function(obj) {
        if (this.is_connected && this.sock && obj) {
            var str = JSON.stringify(obj);
            console.log(str);
            if (str) {
                this.sock.send(str);    
            }
        }
    },

    close: function() {
        if (this.sock !== null) {
            this.sock.close();
            this.sock = null;
        }
        this.is_connected = false;
    }, 

    register_cmd_handler: function(cmd_handers) {
        this.cmd_handler = cmd_handers;
    },
}            

websocket.connect("ws://127.0.0.1:6081/ws");
console.log("connect to server......");
module.exports = websocket;