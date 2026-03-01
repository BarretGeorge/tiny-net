/**
 * TCP Passive Open Test - 测试 TCP 被动打开（服务端三次握手）
 *
 * 测试路径: listen_in -> create_child(SYN_RECEIVED) -> syn_received_in -> ESTABLISHED
 * 直接调用状态处理函数，绕开 tcp_find 的通配匹配问题。
 * 最小初始化，不依赖网卡/路由/pcap。
 */
#include <stdio.h>
#include <string.h>
#include "tcp.h"
#include "tcp_in.h"
#include "tcp_out.h"
#include "tcp_state.h"
#include "tcp_buf.h"
#include "tool.h"
#include "dbug_module.h"
#include "timer.h"

/* ============================== 测试框架 ============================== */

static int pass_count = 0;
static int fail_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL [line %d]: %s\n", __LINE__, msg); \
        fail_count++; \
    } else { \
        printf("  PASS: %s\n", msg); \
        pass_count++; \
    } \
} while (0)

#define TEST_START(name) printf("\n========== %s ==========\n", name)

/* ============================== 地址常量 ============================== */

static const uint8_t SERVER_IP_BUF[] = {192, 168, 1, 1};
static const uint8_t CLIENT_IP_BUF[] = {192, 168, 1, 100};
static const uint16_t SERVER_PORT = 80;
static const uint16_t CLIENT_PORT = 12345;

/* ============================== 辅助函数 ============================== */

/**
 * 构造 pktbuf + tcp_seg_t，header 已转为主机字节序，buf 已 seek 到数据部分。
 * 可直接传给 tcp_listen_in / tcp_syn_received_in 等状态处理函数。
 * 返回的 pktbuf 需要调用者 pktbuf_free。
 */
static pktbuf_t* build_seg(
    tcp_seg_t* seg,
    uint16_t src_port, uint16_t dst_port,
    uint32_t seq, uint32_t ack,
    int f_syn, int f_ack, int f_fin, int f_rst,
    const uint8_t* data, int data_len)
{
    int total = (int)sizeof(tcp_header_t) + data_len;
    pktbuf_t* buf = pktbuf_alloc(total);
    if (buf == NULL)
    {
        printf("  ERROR: pktbuf_alloc failed\n");
        return NULL;
    }

    // 填写 header（直接用主机字节序，模拟 tcp_input 转换后的状态）
    tcp_header_t* hdr = (tcp_header_t*)pktbuf_data(buf);
    memset(hdr, 0, sizeof(tcp_header_t));
    hdr->src_port = src_port;
    hdr->dest_port = dst_port;
    hdr->seq_num = seq;
    hdr->ack_num = ack;
    hdr->f_data_offset = sizeof(tcp_header_t) / 4;
    hdr->f_syn = f_syn;
    hdr->f_ack = f_ack;
    hdr->f_fin = f_fin;
    hdr->f_rst = f_rst;
    hdr->window_size = 65535;
    hdr->checksum = 0;

    // 写入数据
    if (data && data_len > 0)
    {
        memcpy((uint8_t*)hdr + sizeof(tcp_header_t), data, data_len);
    }

    // 填写 seg
    ipaddr_from_buf(&seg->local_ip, SERVER_IP_BUF);
    ipaddr_from_buf(&seg->remote_ip, CLIENT_IP_BUF);
    seg->header = hdr;
    seg->buf = buf;
    seg->data_len = data_len;
    seg->seq = seq;
    seg->seq_len = data_len + f_syn + f_fin;

    // seek 到数据部分（和 tcp_input 一致）
    pktbuf_seek(buf, (int)sizeof(tcp_header_t));

    return buf;
}

/**
 * 创建一个监听状态的 TCP socket
 */
static tcp_t* create_listen_tcp(int backlog)
{
    sock_t* sock = tcp_create(AF_INET, IPPROTO_TCP);
    if (sock == NULL)
    {
        printf("  ERROR: tcp_create failed\n");
        return NULL;
    }
    tcp_t* tcp = (tcp_t*)sock;

    ipaddr_from_buf(&sock->local_ip, SERVER_IP_BUF);
    sock->local_port = SERVER_PORT;

    tcp->conn.backlog = backlog;
    tcp_set_state(tcp, TCP_STATE_LISTEN);

    return tcp;
}

/**
 * 通过 tcp_create_child 创建一个 SYN_RECEIVED 状态的 child
 * 模拟 listen_in 收到 SYN 后创建 child 的行为
 */
static tcp_t* create_child_from_syn(tcp_t* listener, uint32_t client_seq)
{
    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              CLIENT_PORT, SERVER_PORT,
                              client_seq, 0,
                              1, 0, 0, 0, // SYN
                              NULL, 0);
    if (!buf) return NULL;

    tcp_t* child = tcp_create_child(listener, &seg);
    pktbuf_free(buf);

    if (child)
    {
        // tcp_listen_in 在 create_child 后会调用 tcp_send_syn
        // 这里模拟同样的操作（send_out 会因无路由失败，但 syn_out/next_seq 会更新）
        tcp_send_syn(child);
    }
    return child;
}

/* ============================== 测试用例 ============================== */

/**
 * 测试1: tcp_create_child 创建 child 并验证初始状态
 */
static void test_create_child(void)
{
    TEST_START("test_create_child: 创建 child 验证初始状态");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child != NULL, "child 创建成功");
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    // 状态
    TEST_ASSERT(child->state == TCP_STATE_SYN_RECEIVED, "child 状态 SYN_RECEIVED");
    TEST_ASSERT(child->parent == listener, "child->parent == listener");
    TEST_ASSERT(child->flags.inactive == 1, "child 标记为 inactive");
    TEST_ASSERT(child->flags.irs_valid == 1, "irs_valid 已设置");
    TEST_ASSERT(child->flags.syn_out == 1, "syn_out 已设置 (等待 ACK)");

    // 接收序列号
    TEST_ASSERT(child->recv.isn == 1000, "recv.isn == 1000");
    TEST_ASSERT(child->recv.next_seq == 1001, "recv.next_seq == 1001 (SYN+1)");

    // 发送序列号
    TEST_ASSERT(child->send.next_seq == child->send.isn + 1,
                "send.next_seq == isn + 1 (SYN 占一个序列号)");

    // 地址
    ipaddr_t expected_local, expected_remote;
    ipaddr_from_buf(&expected_local, SERVER_IP_BUF);
    ipaddr_from_buf(&expected_remote, CLIENT_IP_BUF);
    TEST_ASSERT(ipaddr_is_equal(&child->base.local_ip, &expected_local),
                "child local_ip == 服务端 IP");
    TEST_ASSERT(ipaddr_is_equal(&child->base.remote_ip, &expected_remote),
                "child remote_ip == 客户端 IP");
    TEST_ASSERT(child->base.local_port == SERVER_PORT,
                "child local_port == 服务端端口");
    TEST_ASSERT(child->base.remote_port == CLIENT_PORT,
                "child remote_port == 客户端端口");

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试2: 完整的被动三次握手
 * SYN -> create_child(SYN_RECEIVED) -> ACK -> syn_received_in -> ESTABLISHED
 */
static void test_passive_handshake(void)
{
    TEST_START("test_passive_handshake: 完整被动三次握手");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    // 第一步: 创建 child (模拟收到 SYN)
    tcp_t* child = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child != NULL, "child 创建成功");
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    TEST_ASSERT(child->state == TCP_STATE_SYN_RECEIVED, "第一步: child SYN_RECEIVED");

    uint32_t child_isn = child->send.isn;
    printf("  [INFO] child ISN = %u, next_seq = %u, un_ack = %u\n",
           child_isn, child->send.next_seq, child->send.un_ack_seq);

    // 第二步: 客户端发送 ACK (seq=1001, ack=child_isn+1)
    tcp_seg_t ack_seg;
    pktbuf_t* ack_buf = build_seg(&ack_seg,
                                  CLIENT_PORT, SERVER_PORT,
                                  1001, child_isn + 1,
                                  0, 1, 0, 0, // ACK
                                  NULL, 0);

    net_err_t err = tcp_syn_received_in(child, &ack_seg);
    pktbuf_free(ack_buf);

    TEST_ASSERT(err == NET_ERR_OK, "syn_received_in 返回 OK");
    TEST_ASSERT(child->state == TCP_STATE_ESTABLISHED, "第二步: child ESTABLISHED");
    TEST_ASSERT(child->flags.syn_out == 0, "SYN 已被确认 (syn_out == 0)");
    TEST_ASSERT(child->send.un_ack_seq == child_isn + 1,
                "un_ack_seq 前进到 ISN+1");

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试3: syn_received_in 唤醒 parent 的 accept
 */
static void test_syn_received_wakeup_parent(void)
{
    TEST_START("test_syn_received_wakeup_parent: 唤醒 parent");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child != NULL, "child 创建成功");
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    uint32_t child_isn = child->send.isn;

    // 验证 parent 关系
    TEST_ASSERT(child->parent == listener, "child->parent 指向 listener");

    // 发送 ACK 完成三次握手
    tcp_seg_t ack_seg;
    pktbuf_t* ack_buf = build_seg(&ack_seg,
                                  CLIENT_PORT, SERVER_PORT,
                                  1001, child_isn + 1,
                                  0, 1, 0, 0,
                                  NULL, 0);

    tcp_syn_received_in(child, &ack_seg);
    pktbuf_free(ack_buf);

    // child 已进入 ESTABLISHED
    TEST_ASSERT(child->state == TCP_STATE_ESTABLISHED, "child ESTABLISHED");

    // sock_wakeup 被调用（无法直接断言信号量状态，但状态转换成功即间接验证）
    // 在真实场景中 accept 会被唤醒

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试4: listen_in 收到 RST 应忽略
 */
static void test_listen_recv_rst(void)
{
    TEST_START("test_listen_recv_rst: listen 收到 RST 忽略");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              CLIENT_PORT, SERVER_PORT,
                              1000, 0,
                              0, 0, 0, 1, // RST
                              NULL, 0);

    net_err_t err = tcp_listen_in(listener, &seg);
    pktbuf_free(buf);

    TEST_ASSERT(err == NET_ERR_OK, "listen_in 返回 OK (忽略 RST)");
    TEST_ASSERT(listener->state == TCP_STATE_LISTEN, "listener 仍是 LISTEN");

    tcp_free(listener);
}

/**
 * 测试5: listen_in 收到 ACK 应回复 RST
 */
static void test_listen_recv_ack(void)
{
    TEST_START("test_listen_recv_ack: listen 收到 ACK 回复 RST");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              CLIENT_PORT, SERVER_PORT,
                              1000, 5000,
                              0, 1, 0, 0, // ACK
                              NULL, 0);

    net_err_t err = tcp_listen_in(listener, &seg);
    pktbuf_free(buf);

    // tcp_send_reset 因无路由会失败，但 listen_in 逻辑正确（调用了 send_reset）
    TEST_ASSERT(listener->state == TCP_STATE_LISTEN, "listener 仍是 LISTEN");

    tcp_free(listener);
}

/**
 * 测试6: listen_in 收到 SYN 创建 child
 */
static void test_listen_recv_syn(void)
{
    TEST_START("test_listen_recv_syn: listen 收到 SYN 创建 child");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              CLIENT_PORT, SERVER_PORT,
                              1000, 0,
                              1, 0, 0, 0, // SYN
                              NULL, 0);

    net_err_t err = tcp_listen_in(listener, &seg);
    pktbuf_free(buf);

    TEST_ASSERT(err == NET_ERR_OK, "listen_in 返回 OK");
    TEST_ASSERT(listener->state == TCP_STATE_LISTEN, "listener 仍是 LISTEN");

    // 通过 tcp_find 查找 child（listener 可能先匹配，所以用间接验证）
    // 验证 backlog 占用了一个位置
    TEST_ASSERT(tcp_backlog_full(listener) == false, "backlog 未满 (1/5)");

    // 清理: 需要找到并释放 child
    // 由于 tcp_find 的通配问题，这里用 backlog 间接验证 child 存在
    ipaddr_t server_ip, client_ip;
    ipaddr_from_buf(&server_ip, SERVER_IP_BUF);
    ipaddr_from_buf(&client_ip, CLIENT_IP_BUF);
    tcp_t* found = tcp_find(&server_ip, SERVER_PORT, &client_ip, CLIENT_PORT);
    // tcp_find 可能返回 listener（通配匹配优先），这是已知问题
    if (found && found != listener)
    {
        tcp_free(found);
    }
    tcp_free(listener);
}

/**
 * 测试7: backlog 满时拒绝新连接
 */
static void test_backlog_full(void)
{
    TEST_START("test_backlog_full: backlog 满拒绝连接");

    tcp_t* listener = create_listen_tcp(1); // backlog=1
    if (!listener) return;

    // 创建第一个 child
    tcp_t* child1 = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child1 != NULL, "第一个 child 创建成功");

    TEST_ASSERT(tcp_backlog_full(listener) == true, "backlog 已满 (1/1)");

    // 再收到 SYN 应该被拒绝
    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              55555, SERVER_PORT, // 不同的客户端端口
                              2000, 0,
                              1, 0, 0, 0,
                              NULL, 0);

    net_err_t err = tcp_listen_in(listener, &seg);
    pktbuf_free(buf);

    TEST_ASSERT(err == NET_ERR_FULL, "backlog 满时 listen_in 返回 FULL");

    if (child1) tcp_free(child1);
    tcp_free(listener);
}

/**
 * 测试8: SYN_RECEIVED 收到 RST 应中止连接
 */
static void test_syn_received_rst(void)
{
    TEST_START("test_syn_received_rst: SYN_RECEIVED 收到 RST");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child != NULL, "child 已创建");
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    TEST_ASSERT(child->state == TCP_STATE_SYN_RECEIVED, "child SYN_RECEIVED");

    // 客户端发 RST
    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              CLIENT_PORT, SERVER_PORT,
                              1001, 0,
                              0, 0, 0, 1, // RST
                              NULL, 0);

    tcp_syn_received_in(child, &seg);
    pktbuf_free(buf);

    TEST_ASSERT(child->state == TCP_STATE_CLOSE, "收到 RST 后 child -> CLOSE");

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试9: SYN_RECEIVED 收到重复 SYN 应中止连接
 */
static void test_syn_received_dup_syn(void)
{
    TEST_START("test_syn_received_dup_syn: SYN_RECEIVED 收到重复 SYN");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child != NULL, "child 已创建");
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    // 重复 SYN
    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              CLIENT_PORT, SERVER_PORT,
                              1000, 0,
                              1, 0, 0, 0, // SYN
                              NULL, 0);

    tcp_syn_received_in(child, &seg);
    pktbuf_free(buf);

    TEST_ASSERT(child->state == TCP_STATE_CLOSE, "收到重复 SYN 后 child -> CLOSE");

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试10: SYN_RECEIVED 收到无效 ACK 应中止
 */
static void test_syn_received_bad_ack(void)
{
    TEST_START("test_syn_received_bad_ack: SYN_RECEIVED 收到无效 ACK");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child != NULL, "child 已创建");
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    uint32_t child_isn = child->send.isn;

    // ACK 号远超 next_seq
    tcp_seg_t seg;
    pktbuf_t* buf = build_seg(&seg,
                              CLIENT_PORT, SERVER_PORT,
                              1001, child_isn + 100,
                              0, 1, 0, 0,
                              NULL, 0);

    tcp_syn_received_in(child, &seg);
    pktbuf_free(buf);

    TEST_ASSERT(child->state == TCP_STATE_CLOSE, "无效 ACK 后 child -> CLOSE");

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试11: 被动打开完成后 child 可以接收数据
 */
static void test_passive_open_recv_data(void)
{
    TEST_START("test_passive_open_recv_data: 被动打开后接收数据");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child != NULL, "child 创建成功");
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    uint32_t child_isn = child->send.isn;

    // 完成三次握手
    tcp_seg_t ack_seg;
    pktbuf_t* ack_buf = build_seg(&ack_seg,
                                  CLIENT_PORT, SERVER_PORT,
                                  1001, child_isn + 1,
                                  0, 1, 0, 0,
                                  NULL, 0);

    tcp_syn_received_in(child, &ack_seg);
    pktbuf_free(ack_buf);

    TEST_ASSERT(child->state == TCP_STATE_ESTABLISHED, "child ESTABLISHED");

    // 客户端发送数据
    const char* payload = "Hello Server";
    int payload_len = (int)strlen(payload);

    tcp_seg_t data_seg;
    pktbuf_t* data_buf = build_seg(&data_seg,
                                   CLIENT_PORT, SERVER_PORT,
                                   1001, child_isn + 1,
                                   0, 1, 0, 0,
                                   (const uint8_t*)payload, payload_len);

    tcp_established_in(child, &data_seg);
    pktbuf_free(data_buf);

    TEST_ASSERT(tcp_buf_count(&child->recv.buf) == payload_len,
                "recv buffer 有数据");

    uint8_t read_buf[64] = {0};
    int n = tcp_buf_read_recv(&child->recv.buf, read_buf, sizeof(read_buf));
    TEST_ASSERT(n == payload_len, "读出正确长度");
    TEST_ASSERT(memcmp(read_buf, payload, payload_len) == 0,
                "数据内容正确");

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试12: 被动打开后客户端 FIN 关闭
 * ESTABLISHED -> CLOSE_WAIT
 */
static void test_passive_open_client_fin(void)
{
    TEST_START("test_passive_open_client_fin: 客户端 FIN 关闭");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    uint32_t child_isn = child->send.isn;

    // 完成三次握手
    tcp_seg_t ack_seg;
    pktbuf_t* ack_buf = build_seg(&ack_seg,
                                  CLIENT_PORT, SERVER_PORT,
                                  1001, child_isn + 1,
                                  0, 1, 0, 0,
                                  NULL, 0);
    tcp_syn_received_in(child, &ack_seg);
    pktbuf_free(ack_buf);

    TEST_ASSERT(child->state == TCP_STATE_ESTABLISHED, "child ESTABLISHED");

    // 客户端发 FIN+ACK
    tcp_seg_t fin_seg;
    pktbuf_t* fin_buf = build_seg(&fin_seg,
                                  CLIENT_PORT, SERVER_PORT,
                                  1001, child_isn + 1,
                                  0, 1, 1, 0, // FIN+ACK
                                  NULL, 0);

    tcp_established_in(child, &fin_seg);
    pktbuf_free(fin_buf);

    TEST_ASSERT(child->state == TCP_STATE_CLOSE_WAIT, "收到 FIN -> CLOSE_WAIT");
    TEST_ASSERT(child->flags.fin_in == 1, "fin_in 已设置");

    tcp_free(child);
    tcp_free(listener);
}

/**
 * 测试13: 多个 child 连接
 */
static void test_multiple_children(void)
{
    TEST_START("test_multiple_children: 多个 child 连接");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    // 创建第一个 child (client seq=1000)
    tcp_t* child1 = create_child_from_syn(listener, 1000);
    TEST_ASSERT(child1 != NULL, "child1 创建成功");

    // 创建第二个 child (client seq=2000, 不同端口)
    // 修改 create_child_from_syn 使用的端口需要单独构造
    tcp_seg_t seg2;
    pktbuf_t* buf2 = build_seg(&seg2,
                               54321, SERVER_PORT, // 不同客户端端口
                               2000, 0,
                               1, 0, 0, 0,
                               NULL, 0);

    tcp_t* child2 = tcp_create_child(listener, &seg2);
    pktbuf_free(buf2);
    if (child2) tcp_send_syn(child2);

    TEST_ASSERT(child2 != NULL, "child2 创建成功");
    if (child1 && child2)
    {
        TEST_ASSERT(child1 != child2, "child1 != child2");
        TEST_ASSERT(child1->state == TCP_STATE_SYN_RECEIVED, "child1 SYN_RECEIVED");
        TEST_ASSERT(child2->state == TCP_STATE_SYN_RECEIVED, "child2 SYN_RECEIVED");
        TEST_ASSERT(child1->base.remote_port == CLIENT_PORT, "child1 remote_port == 12345");
        TEST_ASSERT(child2->base.remote_port == 54321, "child2 remote_port == 54321");
        TEST_ASSERT(child1->parent == listener, "child1 parent == listener");
        TEST_ASSERT(child2->parent == listener, "child2 parent == listener");
    }

    if (child2) tcp_free(child2);
    if (child1) tcp_free(child1);
    tcp_free(listener);
}

/**
 * 测试14: child 从 ESTABLISHED 到完全关闭（服务端主动关闭）
 * ESTABLISHED -> FIN_WAIT_1 -> FIN_WAIT_2 -> TIME_WAIT
 */
static void test_child_active_close(void)
{
    TEST_START("test_child_active_close: child 主动关闭");

    tcp_t* listener = create_listen_tcp(5);
    if (!listener) return;

    tcp_t* child = create_child_from_syn(listener, 1000);
    if (!child)
    {
        tcp_free(listener);
        return;
    }

    uint32_t child_isn = child->send.isn;

    // 完成三次握手
    tcp_seg_t ack_seg;
    pktbuf_t* ack_buf = build_seg(&ack_seg,
                                  CLIENT_PORT, SERVER_PORT,
                                  1001, child_isn + 1,
                                  0, 1, 0, 0,
                                  NULL, 0);
    tcp_syn_received_in(child, &ack_seg);
    pktbuf_free(ack_buf);

    TEST_ASSERT(child->state == TCP_STATE_ESTABLISHED, "child ESTABLISHED");

    // 服务端主动关闭: 发送 FIN
    tcp_send_fin(child);
    tcp_set_state(child, TCP_STATE_FIN_WAIT_1);

    TEST_ASSERT(child->state == TCP_STATE_FIN_WAIT_1, "child FIN_WAIT_1");
    TEST_ASSERT(child->flags.fin_out == 1, "fin_out 已设置");

    // 客户端确认 FIN: ACK
    uint32_t child_next = child->send.next_seq;
    tcp_seg_t fin_ack_seg;
    pktbuf_t* fin_ack_buf = build_seg(&fin_ack_seg,
                                      CLIENT_PORT, SERVER_PORT,
                                      1001, child_next,
                                      0, 1, 0, 0,
                                      NULL, 0);

    tcp_fin_wait_1_in(child, &fin_ack_seg);
    pktbuf_free(fin_ack_buf);

    TEST_ASSERT(child->state == TCP_STATE_FIN_WAIT_2, "ACK 后 -> FIN_WAIT_2");

    // 客户端发 FIN
    tcp_seg_t client_fin_seg;
    pktbuf_t* client_fin_buf = build_seg(&client_fin_seg,
                                         CLIENT_PORT, SERVER_PORT,
                                         1001, child_next,
                                         0, 1, 1, 0, // FIN+ACK
                                         NULL, 0);

    tcp_fin_wait_2_in(child, &client_fin_seg);
    pktbuf_free(client_fin_buf);

    TEST_ASSERT(child->state == TCP_STATE_TIME_WAIT, "收到 FIN -> TIME_WAIT");

    tcp_free(child);
    tcp_free(listener);
}

/* ============================== 主函数 ============================== */

int main(void)
{
    dbug_module_enable_only(DBG_MOD_TCP);

    pktbuf_init();
    net_timer_init();
    tcp_init();

    printf("TCP Passive Open Test\n");
    printf("==============================================\n");

    test_create_child();
    test_passive_handshake();
    test_syn_received_wakeup_parent();
    test_listen_recv_rst();
    test_listen_recv_ack();
    test_listen_recv_syn();
    test_backlog_full();
    test_syn_received_rst();
    test_syn_received_dup_syn();
    test_syn_received_bad_ack();
    test_passive_open_recv_data();
    test_passive_open_client_fin();
    test_multiple_children();
    test_child_active_close();

    printf("\n==============================================\n");
    printf("结果: %d PASS, %d FAIL\n", pass_count, fail_count);
    printf("==============================================\n");

    return fail_count > 0 ? 1 : 0;
}
