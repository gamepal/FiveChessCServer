var websocket = require("websocket");

cc.Class({
    extends: cc.Component,

    properties: {
        black_spriteframe: {
            default: null,
            type: cc.SpriteFrame,
        },
        
        white_spriteframe: {
            default: null,
            type: cc.SpriteFrame,
        },

        sprite_Array :{
            default:[],
            type:[cc.SpriteFrame],
        },
    }, 

    // use this for initialization
    onLoad: function () {

        this.local_seatid = -1; // 服务器给本方玩家seatid

        var services_handlers = {
            2: this.on_game_service_entry.bind(this),  //key服务 value函数
        };        
        websocket.register_cmd_handler(services_handlers);

        this.seat_a = cc.find("UI_ROOT/seat_a");
        this.seat_b = cc.find("UI_ROOT/seat_b");
        this.face_a = this.seat_a.getChildByName("face").getComponent(cc.Sprite);  
        this.face_b = this.seat_b.getChildByName("face").getComponent(cc.Sprite);

        this.seat_a.active = false;
        this.seat_b.active = false;

        this.clear_the_seat(); 
        this.banker_id = -1;
        this.cur_sv_id = -1;
        // 棋盘的中心点
        this.CENTER_X = 7; // 从0开始
        this.CENTER_Y = 7;
        
        this.BLOCK_W = 41;  //格子宽
        this.BLOCK_H = 41;

        this.chess_disk = cc.find("UI_ROOT/chessbox");
        // 下棋，测试下棋
        this.chess_disk.on(cc.Node.EventType.TOUCH_START, function(t) {
            if (this.cur_sv_id != this.local_seatid) { //对手下棋 避免向服务器发送数据
                return;    
            }
            
            var w_pos = t.getLocation(); // 世界坐标
            var pos = this.chess_disk.convertToNodeSpaceAR(w_pos);  //局部空间坐标系 坐标系以锚点为原点
            var block_x = Math.floor((pos.x + (this.BLOCK_W * 0.5)) / this.BLOCK_W) + this.CENTER_X;
            var block_y = Math.floor((pos.y + (this.BLOCK_H * 0.5)) / this.BLOCK_H) + this.CENTER_Y;
            
            
            // 发送下棋的命令给服务器
            var req_data = {
                0: 2, // 服务号,
                1: 6, // 下棋的命令
                2: block_x,  //下的位置-7~7整数
                3: block_y, 
            };
            websocket.send_object(req_data);
            // end 
            
            
        }.bind(this), this.chess_disk);

        this.checkout_vectory = cc.find("UI_ROOT/tx_end_vectory");
        this.checkout_fail = cc.find("UI_ROOT/tx_end_fail");
        this.checkout_vectory.active = false;
        this.checkout_fail.active = false;            
    },

    // 游戏开始或结束清理座位
    clear_the_seat: function() {
        this.seat_a.getChildByName("time_bar").active = false;
        this.seat_b.getChildByName("time_bar").active = false;
    },

    // {stype, opt_cmd, status, seatid}
    sitdown: function(cmd) {
        if (cmd[2] != 1) { // status != OK
            console.log("######sit down error", cmd);
            return;
        }
        
        this.local_seatid = cmd[3];
        // 本方座位就要显示出来
        this.seat_a.active = true;

        //sprite重置
        this.face_a.spriteFrame = this.sprite_Array[0]; 
        this.seat_a.getChildByName("time_bar").active = false;   
        // end 
    },
    
    // 服务器主动发送，没有状态码
    user_arrived: function(cmd) {
        var sv_seatid = cmd[2];
        console.log("######sv_seatid is arrived", sv_seatid);
        console.log("local", this.local_seatid);

        this.seat_b.active = true;

        if (1 == sv_seatid) {
            this.face_a.spriteFrame = this.sprite_Array[1];
            this.face_b.spriteFrame = this.sprite_Array[2];            
        }
        else
        {
            this.face_a.spriteFrame = this.sprite_Array[2];
            this.face_b.spriteFrame = this.sprite_Array[1];  
        }
    },
    
    player_give_chess: function(cmd) {
        if (cmd[2] != 1) { //  非法操作,OK
            console.log("######user_standup error", cmd);
            return;
        }
        
        var sv_seatid = cmd[3];
        var block_x = cmd[4];
        var block_y = cmd[5];
        this.give_chess_at(block_x, block_y, sv_seatid);
    }, 

    give_chess_at: function(block_x, block_y, sv_seatid)  {
        // 下一个棋子
        
        var sp = null;
        if (sv_seatid == this.banker_id) {
            sp = this.black_spriteframe;    
        }
        else {
            sp = this.white_spriteframe;
        }
        
        var node = new cc.Node();
        var s_com = node.addComponent(cc.Sprite);
        s_com.spriteFrame = sp;
        
        var xpos = (block_x - this.CENTER_X) * this.BLOCK_W;
        var ypos = (block_y - this.CENTER_Y) * this.BLOCK_H;          
        this.chess_disk.addChild(node);
        node.x = xpos;
        node.y = ypos;
        node.scale = 2;
        // end 
    },    

    user_standup: function(cmd) {
        if (cmd[2] != 1) { // 
            console.log("######user_standup error", cmd);
            return;
        }
        var sv_seatid = cmd[3];
        if (sv_seatid == this.local_seatid) { // 自己站起来
            this.seat_a.active = false;
            this.seat_b.active = false;
        }
        else { // 其它的玩家站起
            this.seat_b.active = false;
        }



        this.chess_disk.removeAllChildren();
    },
    
    
    game_stated: function(cmd) {
        this.banker_id = cmd[2]; // 庄家(先下)id
        this.cur_sv_id = -1;
        
        // 指示那个先下，
        // end 
        
        // 播放开始动画
        // end 
        
        // 把上一局的棋子清理掉
        this.chess_disk.removeAllChildren();
        // end 
    },   

    turn_to_player: function(cmd) {
        var sv_seatid = cmd[2];
        this.round_turn_to_player(sv_seatid);
        
        this.cur_sv_id = sv_seatid;        
    },

    // 当轮到这个玩家的时候来显示和隐藏这个状态
    round_turn_to_player: function(sv_seatid) {
        if (this.local_seatid == sv_seatid) {
            this.seat_a.getChildByName("time_bar").active = true;
            this.seat_b.getChildByName("time_bar").active = false;
        }
        else {
            this.seat_a.getChildByName("time_bar").active = false;
            this.seat_b.getChildByName("time_bar").active = true;
        }
    },

    checkout_game: function(cmd) {
        var winner = cmd[2];
        if (winner == this.local_seatid) { // 等于自己,播放胜利动画1s
            this.checkout_vectory.active = true;
            this.scheduleOnce(function() {
                this.checkout_vectory.active = false;
            }.bind(this), 1);
        }
        else { // 播放失败动画1s
            this.checkout_fail.active = true;
            this.scheduleOnce(function() {
                this.checkout_fail.active = false;
            }.bind(this), 1); 
        }
        this.node.getChildByName('sitdown').getComponent(cc.Button).interactable = false;          
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
            case 4: // GAME_START, 表示游戏开始了。
            {
                this.game_stated(cmd);
            }
            break;
            case 5: // 轮到某个玩家,
            {
                this.turn_to_player(cmd);
            }
            break;
            case 6: // 某个玩家的下棋操作
            {
                this.player_give_chess(cmd);
            }
            break;
            case 7: // 游戏结束
            {
                this.checkout_game(cmd);
            }
            break;
            case 8: // USER_QUIT
            {
                // 1.1秒以后清空牌局，等待重新点击进入
                this.scheduleOnce(function() {
                    this.seat_a.active = false;
                    this.seat_b.active = false;
                    
                    this.checkout_vectory.active = false;
                    this.checkout_fail.active = false;
                    this.chess_disk.removeAllChildren();
                    this.node.getChildByName('sitdown').getComponent(cc.Button).interactable = true;
                }.bind(this), 1.1);
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
