var websocket = require("websocket");

cc.Class({
    extends: cc.Component,

    properties: {
        // foo: {
        //    default: null,      // The default value will be used only when the component attaching
        //                           to a node for the first time
        //    url: cc.Texture2D,  // optional, default is typeof default
        //    serializable: true, // optional, default is true
        //    visible: true,      // optional, default is true
        //    displayName: 'Foo', // optional
        //    readonly: false,    // optional, default is false
        // },
        // ...
    }, 

    // use this for initialization
    onLoad: function () {

        this.sv_seatid = -1; // 服务器给本方玩家seatid

        var services_handlers = {
            2: this.on_game_service_entry.bind(this),  //key服务 value函数
        };        
        websocket.register_cmd_handler(services_handlers);

        this.seat_a = cc.find("UI_ROOT/seat_a");
        this.seat_a.active = false;
        
        this.seat_b = cc.find("UI_ROOT/seat_b");
        this.seat_b.active = false;   

        this.clear_the_seat();     
    },

    // 游戏开始或结束清理座位
    clear_the_seat: function() {
        this.seat_a.getChildByName("time_bar").active = false;
        this.seat_b.getChildByName("time_bar").active = false;
    },

    // 当轮到这个玩家的时候来显示和隐藏这个状态
    seat_turn_to_player: function(sv_seatid) {
        if (this.sv_seatid == sv_seatid) {
            this.seat_a.getChildByName("time_bar").active = true;
            this.seat_b.getChildByName("time_bar").active = false;
        }
        else {
            this.seat_a.getChildByName("time_bar").active = false;
            this.seat_b.getChildByName("time_bar").active = true;
        }
    },

    // {stype, opt_cmd, status, seatid}
    sitdown: function(cmd) {
        if (cmd[2] != 1) { // status != OK
            console.log("######sit down error", cmd);
            return;
        }
        
        this.sv_seatid = cmd[3];
        // 本方座位就要显示出来
        this.seat_a.active = true;
        // end 
    },
    
    // 服务器主动发送，没有状态码
    user_arrived: function(cmd) {
        var sv_seatid = cmd[2];
        console.log("######sv_seatid is arrived", sv_seatid);
        
        this.seat_b.active = true;
    },
    
    user_standup: function(cmd) {
        if (cmd[2] != 1) { // 
            console.log("######user_standup error", cmd);
            return;
        }
        var sv_seatid = cmd[3];
        if (sv_seatid == this.sv_seatid) { // 自己站起来
            this.seat_a.active = false;
            this.seat_b.active = false;
        }
        else { // 其它的玩家站起
            this.seat_b.active = false;
        }
    },
    

    on_game_service_entry: function(cmd) {
        var opt_type = cmd[1];
        switch(opt_type) {
            case 1: // SITDOWN
                this.sitdown(cmd);
            break;
            case 2: // USERARRIVE
            {
                this.user_arrived(cmd);
            }
            break;
            case 3: // USERSTANDUP
            {
                this.user_standup(cmd);
            }
            break;
        }
    }, 

    // called every frame, uncomment this function to activate update callback
    // update: function (dt) {

    // },

    // 玩家站起
    on_standup_click: function() {
        var req_data = {
            0: 2, // 服务号,
            1: 3, // 用户站起STANDUP
        };
        websocket.send_object(req_data); 
    },

    on_start_click:function() {
        var req_data = {
            0: 2, //服务类型
            1: 1, //操作命令 SITDOWN 
            //....用户数据 如多个桌子 还需要桌子id
        };

        websocket.send_object(req_data);
    }
});
