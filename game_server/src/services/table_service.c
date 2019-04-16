#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "service_type.h"
#include "table_service.h"

enum {
	OK = 1, // 成功了
	INVALID_COMMAND = -100, // 无效的操作命令
	INVALID_PARAMS = -101, // 无效的用户参数;
	USER_IS_INTABLE = -102, // 玩家已经在桌子里，不能重复的添加;
	TABLE_IS_FULL = -103, // 当前桌子已满，请稍后再试;
	USER_IS_NOT_INTABLE = -104, //当前的玩家不在桌子里面
	INAVLID_TABLE_STATUS = -105, // 错误的桌子状态;
	INVALID_ACTION = -106, // 非法操作
};

enum {
	BLACK_CHESS = 1, // 表示的是黑色的棋子;
	WHITE_CHESS = 2, // 表色的是白色的棋子;
};

enum {
	SITDOWN = 1, // 玩家坐下命令
	USER_ARRIVED = 2, // 有玩家进来; 
	STANDUP = 3, // 玩家站起;
	GAME_STARTED = 4, // 游戏开始;
	TURN_TO_PLAYER = 5, // 轮到哪个玩家;
	GIVE_CHESS = 6, // 下棋的命令
	CHECKOUT = 7, // 结算
	USER_QUIT = 8, // 用户离开
};
 
enum {
	T_READY = 0, // 随时准备开始游戏; 
	T_STEADY, // 游戏已经开始，但是，还要等一会才能正式开始; 留给客户端播动画
	T_PLAYING, // 游戏正在进行中;
	T_CHECKOUT, // 游戏正在结算中;
};

// 玩家踢出的原因
enum {
	GAME_OVER = 1,
	PLAY_LOST = 2,
	PLAY_STANDUP = 3, // 离开,逃跑
};
// end 

// 保存session对象来替代,没有玩家数据这么一说
typedef struct session game_player;

struct game_seat {
	int is_sitdown; // 是否为一个有效的桌子;
	int seat_id; // 座位的id号
	// 玩家的对象
	game_player* player_session;
};


// 五子棋的桌子
#define PLAYER_NUM 2
#define DISK_SIZE 15

struct game_table {
	int status; // 存放我们当前的桌子的状体,初始化是TRADEAD的状态;
	int banker_id; // 我们当前那个玩家持黑先下。
	int cur_turn;
	struct game_seat seats[PLAYER_NUM];

	int chess_disk[DISK_SIZE][DISK_SIZE];  //15×15棋盘
};

static void
write_error_command(struct session*s, int opt, int status) {
	json_t* json = json_new_comand(FIVE_CHESS_SERVICE, opt);
	json_object_push_number(json, "2", status);
	session_send_json(s, json);
	json_free_value(&json);
}

static void
table_broadcast_json(struct game_table* table, json_t* json, int not_to_seatid) {
	for (int i = 0; i < PLAYER_NUM; i++) {
		if (table->seats[i].is_sitdown && i != not_to_seatid) {
			session_send_json(table->seats[i].player_session, json);
		}
	}
}

static int
is_in_table(struct game_table* table, game_player* p_session) {
	for (int i = 0; i < PLAYER_NUM; i++) {
		if (table->seats[i].is_sitdown && table->seats[i].player_session == p_session) {
			return 1;
		}
	}
	return 0;
}


static int
get_seatid(struct game_table* table, game_player* p_session) {  //player在table第i号seat坐下
	for (int i = 0; i < PLAYER_NUM; i++) {
		if (table->seats[i].is_sitdown && table->seats[i].player_session == p_session) {
			return i;
		}
	}
	return -1;
}

static int
get_empty_seatid(struct game_table* table, game_player* p_session) {
	for (int i = 0; i < PLAYER_NUM; i++) {
		if (table->seats[i].is_sitdown == 0) {
			return i;
		}
	}
	return -1;
}


static json_t*
get_user_arrived_data(struct game_table* table, int seatid) {
	json_t* json = json_new_comand(FIVE_CHESS_SERVICE, USER_ARRIVED);
	// 服务器主动发给客户端的，无需要写status码;
	// 玩家的数据，由于没有玩家的数据，所以我们用seatid来代替;
	json_object_push_number(json, "2", seatid);
	return json;
}

static void
sitdown_success(struct game_table* table, game_player* p_session, int seatid) {
	// 要将其它玩家的数据发送给这个玩家 {2 seatid}
	for (int i = 0; i < PLAYER_NUM; i++) {
		if (i != seatid && table->seats[i].is_sitdown == 1) {
			json_t* json = get_user_arrived_data(table, i);
			session_send_json(p_session, json);
			json_free_value(&json);
		}
	}
	// end 
}

static int
check_game_started(struct game_table* table) {
	for (int i = 0; i < PLAYER_NUM; i++) {
		if (table->seats[i].is_sitdown == 0) {
			return 0;
		}
	}
	return 1;
}

static void
turn_to_player(struct game_table* table, int seatid) {
	table->cur_turn = seatid;
	json_t* json = json_new_comand(FIVE_CHESS_SERVICE, TURN_TO_PLAYER);
	json_object_push_number(json, "2", seatid);
	table_broadcast_json(table, json, -1);
	json_free_value(&json);
}

static void
start_game(struct game_table* table) {
	table->status = T_STEADY; // 桌子已准备好了。
	table->banker_id = rand() % 2; // [0, 1]随机一个持黑先行的
	memset(table->chess_disk, 0, sizeof(int) * DISK_SIZE * DISK_SIZE);  //清空棋盘

	// 广播一个游戏开始的命令给我们的桌子内的客户端，通知他们游戏要开始
	// 客户端收到这个命令以后，播放ready的动画,在正式开始之前先预留一个时间段，过了这个时间段后，才开始
	json_t* json = json_new_comand(FIVE_CHESS_SERVICE, GAME_STARTED);
	json_object_push_number(json, "2", table->banker_id); // 发送给客户端，让客户端去判断自己是持黑还是持白色;
	table_broadcast_json(table, json, -1); // 所有的玩家都要收到这个广播;
	json_free_value(&json);
	// end 

	// 真正的轮到玩家开始。
	table->status = T_PLAYING;
	turn_to_player(table, table->banker_id); // 轮到庄家开始 庄家持黑；
	// end 
}

// 1就是表示分出胜负, 0就是表示未分出胜负
static int
checkout_game(struct game_table* table, int seatid) {
	int value = (table->banker_id == seatid) ? BLACK_CHESS : WHITE_CHESS;  //黑棋还是白棋

	// 横向扫描，看是否有五个棋子连接起来；
	for (int line = 0; line < DISK_SIZE; line++) { // 遍历行
		for (int col = 0; col < DISK_SIZE - 4; col++) { // 遍历列
			if (table->chess_disk[line][col + 0] == value &&
				table->chess_disk[line][col + 1] == value &&
				table->chess_disk[line][col + 2] == value &&
				table->chess_disk[line][col + 3] == value &&
				table->chess_disk[line][col + 4] == value) {
				return 1;
			}
		}
	}
	// end 

	// 竖向扫描
	for (int col = 0; col < DISK_SIZE; col++) {
		for (int line = 0; line < DISK_SIZE - 4; line++) {
			if (table->chess_disk[line + 0][col] == value &&
				table->chess_disk[line + 1][col] == value &&
				table->chess_disk[line + 2][col] == value &&
				table->chess_disk[line + 3][col] == value &&
				table->chess_disk[line + 4][col] == value) {
				return 1;
			}
		}
	}
	// end 

	// 反斜杠一半的方向开始 
	// 下半部
	for (int y = 0; y < DISK_SIZE; y++) {
		int start_x = 0;
		int start_y = y;
		while (1) {
			if (start_y < 4 || start_x + 4 >= DISK_SIZE) { // 开始的对角线不足五个
				break;
			}

			if (table->chess_disk[start_y - 0][start_x + 0] == value &&
				table->chess_disk[start_y - 1][start_x + 1] == value &&
				table->chess_disk[start_y - 2][start_x + 2] == value &&
				table->chess_disk[start_y - 3][start_x + 3] == value &&
				table->chess_disk[start_y - 4][start_x + 4] == value) {
				return 1;
			}
			// 对角线的下一个起点
			start_y--;
			start_x++;

		}
	}

	// 上半部
	for (int x = 1; x < DISK_SIZE - 4; x++) {
		int start_x = x;
		int start_y = DISK_SIZE - 1;

		while (1) {
			if (start_y < 4 || start_x + 4 >= DISK_SIZE) { // 开始的对角线不足五个
				break;
			}

			if (table->chess_disk[start_y - 0][start_x + 0] == value &&
				table->chess_disk[start_y - 1][start_x + 1] == value &&
				table->chess_disk[start_y - 2][start_x + 2] == value &&
				table->chess_disk[start_y - 3][start_x + 3] == value &&
				table->chess_disk[start_y - 4][start_x + 4] == value) {
				return 1;
			}
			// 对角线的下一个起点
			start_y--;
			start_x++;

		}
	}
	// end 

	// 斜杠部分
	// 右半部分
	for (int y = 0; y < DISK_SIZE; y++) {
		int start_x = DISK_SIZE - 1;
		int start_y = y;
		while (1) {
			if (start_y < 4 || start_x < 4) { // 开始的对角线不足五个
				break;
			}

			if (table->chess_disk[start_y - 0][start_x - 0] == value &&
				table->chess_disk[start_y - 1][start_x - 1] == value &&
				table->chess_disk[start_y - 2][start_x - 2] == value &&
				table->chess_disk[start_y - 3][start_x - 3] == value &&
				table->chess_disk[start_y - 4][start_x - 4] == value) {
				return 1;
			}
			// 对角线的下一个起点
			start_y--;
			start_x--;

		}
	}

	// 左半部分
	for (int x = DISK_SIZE - 1; x >= 0; x--) {
		int start_x = x;
		int start_y = DISK_SIZE - 1;

		while (1) {
			if (start_y < 4 || start_x < 4) { // 开始的对角线不足五个
				break;
			}

			if (table->chess_disk[start_y - 0][start_x - 0] == value &&
				table->chess_disk[start_y - 1][start_x - 1] == value &&
				table->chess_disk[start_y - 2][start_x - 2] == value &&
				table->chess_disk[start_y - 3][start_x - 3] == value &&
				table->chess_disk[start_y - 4][start_x - 4] == value) {
				return 1;
			}
			// 对角线的下一个起点
			start_y--;
			start_x--;

		}
	}
	// end 
	return 0;
}

static json_t*
get_user_quit_data(int seatid, int reason) {
	json_t* json = json_new_comand(FIVE_CHESS_SERVICE, USER_QUIT);
	json_object_push_number(json, "2", seatid);
	json_object_push_number(json, "3", reason);

	return json;
}

// 服务器广播给所有的客户端玩家的
// { FIVE_CHESS_SERVICE, USER_QUIT, seadid, reason}
static void do_user_quit(struct game_table* table, int seatid, int reason) {
	if (table->seats[seatid].is_sitdown == 0) {
		return;
	}

	json_t* json = get_user_quit_data(seatid, reason);
	// 广播给所有的玩家，它离开了
	table_broadcast_json(table, json, -1);
	// end 
	json_free_value(&json);

	table->seats[seatid].is_sitdown = 0;
	table->seats[seatid].player_session = NULL;
}

static void
send_checkout(struct game_table* table, int winner) {
	table->status = T_CHECKOUT;

	json_t* json = json_new_comand(FIVE_CHESS_SERVICE, CHECKOUT);
	json_object_push_number(json, "2", winner);
	table_broadcast_json(table, json, -1);
	json_free_value(&json);

	do_user_quit(table, 0, GAME_OVER);
	do_user_quit(table, 1, GAME_OVER);

	table->status = T_READY;
}

static void
on_player_give_chess(struct game_table* table, struct session* s, json_t* json, int len) {
	if (len != 4) { // 0: 2, , 1: 6,  2 : block_x, 3 : block_y,
		write_error_command(s, GIVE_CHESS, INVALID_PARAMS);
		return;
	}

	// 桌子的状态
	if (table->status != T_PLAYING) {
		write_error_command(s, GIVE_CHESS, INVALID_ACTION);
		return;
	}
	// end 

	// 玩家是否在桌子里面
	if (!is_in_table(table, s)) {
		write_error_command(s, GIVE_CHESS, USER_IS_INTABLE);
		return;
	}
	// end 
	int seatid = get_seatid(table, s);
	if (seatid != table->cur_turn) {// 当前是否轮到了改玩家说话
		write_error_command(s, GIVE_CHESS, INVALID_ACTION);
		return;
	}
	// end 

	// 获得我们玩家传给我们的下棋的位置的坐标;
	json_t* value = json_object_at(json, "2");
	if (!value || value->type != JSON_NUMBER) {
		write_error_command(s, GIVE_CHESS, INVALID_PARAMS);
		return;
	}
	int x_block = atoi(value->text);

	value = json_object_at(json, "3");
	if (!value || value->type != JSON_NUMBER) {
		write_error_command(s, GIVE_CHESS, INVALID_PARAMS);
		return;
	}
	int y_block = atoi(value->text);

	// 检测用户的棋盘位置的合法性
	if (x_block < 0 || x_block >= DISK_SIZE || y_block < 0 || y_block >= DISK_SIZE) {
		write_error_command(s, GIVE_CHESS, INVALID_PARAMS);
		return;
	}
	// end

	// 检查这个地方有没有已经被下过棋，如果有，也是非法的操作
	if (table->chess_disk[y_block][x_block] != 0) {
		write_error_command(s, GIVE_CHESS, INVALID_PARAMS);
		return;
	}
	// end 

	// 总算正确了
	if (seatid == table->banker_id) { // 黑色
		table->chess_disk[y_block][x_block] = BLACK_CHESS; // 
	}
	else { // 白色
		table->chess_disk[y_block][x_block] = WHITE_CHESS;  
	}

	// 将这个用户操作广播回去，
	json_t* send_data = json_new_comand(FIVE_CHESS_SERVICE, GIVE_CHESS);
	json_object_push_number(send_data, "2", OK);
	json_object_push_number(send_data, "3", seatid);
	json_object_push_number(send_data, "4", x_block);
	json_object_push_number(send_data, "5", y_block);
	table_broadcast_json(table, send_data, -1); // 广播给桌子里面所有的人
	json_free_value(&send_data);
	// end 

	// 结算
	if (checkout_game(table, seatid)) { // 游戏结算流程，发送结算结果
		send_checkout(table, seatid);
		return;
	}
	// end 

	// 转入下一个玩家
	int next = (table->cur_turn + 1);
	if (next >= PLAYER_NUM) {
		next = 0;
	}
	turn_to_player(table, next);
	// end 
}


static void
on_player_sitdown(struct game_table* table, struct session* s, json_t* json, int len) {
	if (len != 2) { // 命令的协议，必须严格遵守
		write_error_command(s, SITDOWN, INVALID_PARAMS);
		return;
	}

	// 判断一下玩家是否已经在桌子里面了
	if (is_in_table(table, s)) {
		write_error_command(s, SITDOWN, USER_IS_INTABLE);
		return;
	}
	// end 

	// 判断当前桌子的状态，如果桌子是游戏中，那么我们就直接不能坐下了。
	if (table->status != T_READY) {
		write_error_command(s, SITDOWN, INAVLID_TABLE_STATUS);
		return;
	}
	// end 

	// 给这个玩家尝试着找个空位
	int seatid = get_empty_seatid(table, s);
	if (seatid < 0 || seatid >= PLAYER_NUM) {
		write_error_command(s, SITDOWN, TABLE_IS_FULL);
		return;
	}
	// end 

	// 保存数据到座位
	table->seats[seatid].player_session = s;
	table->seats[seatid].seat_id = seatid;
	table->seats[seatid].is_sitdown = 1;
	// end 

	// 发送给客户端，表示你坐下成功了。
	// {stype, opt_cmd, status, seatid};
	json_t* response = json_new_comand(FIVE_CHESS_SERVICE, SITDOWN);
	json_object_push_number(response, "2", OK);
	json_object_push_number(response, "3", seatid);
	session_send_json(s, response);
	json_free_value(&response);
	// end 

	// 发送其它的玩家的数据给当前坐下的玩家;
	sitdown_success(table, s, seatid);
	// end 

	// 要广播给桌子里面的其他所有玩家， 我这个玩家进来了，
	json_t* u_arrived = get_user_arrived_data(table, seatid);
	//  广播给这个桌子的其它玩家,不要再广播回来自己了。
	table_broadcast_json(table, u_arrived, seatid);
	// 
	json_free_value(&u_arrived);
	// end 

	// 检查游戏是否开始
	if (check_game_started(table)) {
		start_game(table);
	}
	// end 
}

static void
on_player_standup(struct game_table* table, struct session* s, json_t* json, int len) {
	if (len < 2) {
		write_error_command(s, STANDUP, INVALID_PARAMS);
		return;
	}

	// 玩家是否在桌子里面
	if (!is_in_table(table, s)) {
		write_error_command(s, STANDUP, USER_IS_NOT_INTABLE);
		return;
	}
	// end 

	// 如果是游戏中，结算...
	// end 

	int seatid = get_seatid(table, s);
	table->seats[seatid].is_sitdown = 0; // 空位
	table->seats[seatid].player_session = NULL;
	table->seats[seatid].seat_id = -1;

	// 玩家离开了，我们要吧这个消息广播给其它的玩家，也包括自己
	json_t* standup_data = json_new_comand(FIVE_CHESS_SERVICE, STANDUP);
	json_object_push_number(standup_data, "2", OK);
	json_object_push_number(standup_data, "3", seatid);
	session_send_json(s, standup_data);
	table_broadcast_json(table, standup_data, -1); // 广播给所有的人。
	// end 

	if (table->status == T_PLAYING) {
		// 结算
		table->status = T_CHECKOUT;
		int winner = seatid + 1;
		if (winner >= PLAYER_NUM) {
			winner = 0;
		}
		send_checkout(table, winner);
		// end
	}
	// 继续开始,零食
	table->status = T_READY;
	// end 
}

static void
init_service_module(struct service_module* module) {
	struct game_table* table = malloc(sizeof(struct game_table));
	memset(table, 0, sizeof(struct game_table));
	module->module_data = table;
}


static int
on_five_chess_cmd(void* module_data, struct session* s,
	json_t* json, unsigned char* data, int len) {
	struct game_table* table = (struct game_table*)module_data;
	int size = json_object_size(json);
	if (size < 2) { // 直接返回 key：value
		return 0;
	}

	json_t* j_opt_cmd = json_object_at(json, "1");
	if (j_opt_cmd == NULL || j_opt_cmd->type != JSON_NUMBER) {
		return 0;
	}
	int opt_cmd = atoi(j_opt_cmd->text);

	switch (opt_cmd) {
		case SITDOWN:
		{
			on_player_sitdown(table, s, json, size);
		}
		break;
		case STANDUP:
		{
			on_player_standup(table, s, json, size);
		}
		break;
		case GIVE_CHESS:
		{
			on_player_give_chess(table, s, json, size);
		}
		break;
		default: // 无效的操作
		{
			write_error_command(s, opt_cmd, INVALID_COMMAND);
		}
		break;
	}
	return 0;
}

static void
on_player_lost(void* module_data, struct session* s) {
	struct game_table* table = (struct game_table*)module_data;
	int seatid = get_seatid(table, s);
	if (seatid < 0 || seatid >= PLAYER_NUM) { // 这个不在桌子上;
		return;
	}

	// 玩家离开
	table->seats[seatid].player_session = NULL;
	table->seats[seatid].is_sitdown = 0;
	table->seats[seatid].seat_id = -1;
	// end 

	// 广播
	// 玩家离开了，我们要吧这个消息广播给其它的玩家
	json_t* standup_data = json_new_comand(FIVE_CHESS_SERVICE, STANDUP);
	json_object_push_number(standup_data, "2", OK);
	json_object_push_number(standup_data, "3", seatid);
	table_broadcast_json(table, standup_data, -1); // 广播给所有的人。
	// end 

	if (table->status == T_PLAYING) {
		// 结算
		table->status = T_CHECKOUT;
		int winner = seatid + 1;
		if (winner >= PLAYER_NUM) {
			winner = 0;
		}
		send_checkout(table, winner);
		// end
	}

	// 继续开始
	table->status = T_READY;
	// end 

	return;
}


struct service_module SERVICE_TABLE = {
	FIVE_CHESS_SERVICE,
	init_service_module, // 注册完成后的初始化 *init_service_module
	NULL, // 二进制数据协议
	on_five_chess_cmd, // json数据协议 *on_json_protocal_recv
	on_player_lost, // 链接丢失 *on_connect_lost
	NULL, // 用户数据
};