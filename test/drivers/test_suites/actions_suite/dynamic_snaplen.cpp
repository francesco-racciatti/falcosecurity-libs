#include "../../event_class/event_class.h"
#include <capture_macro.h>

#ifdef __NR_write
TEST(Actions, dynamic_snaplen_negative_fd)
{
	auto evt_test = get_syscall_event_test(__NR_write, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	/* Here we have a len greater than the default snaplen, thanks to the dynamic snaplen
	 * we could be able to retrieve all `DEFAULT_SNAPLEN * 2` bytes but since the fd is negative
	 * we cannot enable this logic.
	 */
	int fd = -1;
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "HTTP/\0";
	ssize_t write_bytes = syscall(__NR_write, fd, (void *)buf, data_len);
	assert_syscall_state(SYSCALL_FAILURE, "write", write_bytes);
	int64_t errno_value = -errno;

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)errno_value);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	/* We expect the default snaplen since we cannot enable the dynamic snaplen logic */
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_no_socket)
{
	auto evt_test = get_syscall_event_test(__NR_write, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	/* Here we have a len greater than the default snaplen, thanks to the dynamic snaplen
	 * we could be able to retrieve all `DEFAULT_SNAPLEN * 2` bytes but since the fd is not
	 * a socket we cannot enable this logic.
	 */
	int fd = (2 ^ 16) - 1;
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "HTTP/\0";
	ssize_t write_bytes = syscall(__NR_write, fd, (void *)buf, data_len);
	assert_syscall_state(SYSCALL_FAILURE, "write", write_bytes);
	int64_t errno_value = -errno;

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)errno_value);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	/* We expect the default snaplen since we cannot enable the dynamic snaplen logic */
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}
#endif

#if defined(__NR_sendto) && defined(__NR_socket) && defined(__NR_shutdown) && defined(__NR_close) && defined(__NR_bind) && defined(__NR_listen) && defined(__NR_connect)
TEST(Actions, dynamic_snaplen_HTTP)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "HTTP/\0";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN * 2);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_partial_HTTP_OPT)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	/* This is not recognized in the dynamic snaplen logic */
	char buf[data_len] = "OP\0";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_HTTP_TRACE)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "TRACE\0";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN * 2);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_MYSQL)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr, PPM_PORT_MYSQL);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "simple message";
	/* This should trigger our SQL detection logic, `3` is used as ASCII code */
	buf[3] = 3;
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN * 2);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_not_MYSQL)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr, PPM_PORT_MYSQL);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "1111\0";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_POSTGRES)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr, PPM_PORT_POSTGRES);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	/* This should trigger our Postgres detection logic, `0` is used as ASCII code */
	char buf[data_len] = "P2434242\0";
	buf[1] = 0;
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN * 2);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_not_POSTGRES)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr, PPM_PORT_POSTGRES);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "00\0";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_MONGO)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = {0};
	*(int32_t *)(&buf[12]) = 0x01; // this 1 and it's ok
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN * 2);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_not_MONGO)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = {0};
	*(int32_t *)(&buf[12]) = 0x07;
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_fullcapture_port_range)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	/* The server's port is in the range so the dynamic snaplen should be enabled */
	evt_test->set_fullcapture_port_range(IPV4_PORT_SERVER, IPV4_PORT_SERVER);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "simple message";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	/* we need to clean the values after we read our event because the kernel module
	 * flushes the ring buffers when we change this config.
	 */
	evt_test->set_fullcapture_port_range(0, 0);

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN * 2);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_statsd_port)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	/* The server's port is the statsd port so the dynamic snaplen should be enabled */
	evt_test->set_statsd_port(IPV4_PORT_SERVER);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "simple message";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	/* we need to clean the values after we read our event because the kernel module
	 * flushes the ring buffers when we change this config.
	 */
	evt_test->set_statsd_port(0);

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN * 2);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(Actions, dynamic_snaplen_no_statsd_port)
{
	auto evt_test = get_syscall_event_test(__NR_sendto, EXIT_EVENT);

	evt_test->set_do_dynamic_snaplen(true);

	/* The server's port is not the statsd port so the dynamic snaplen shouldn't be enabled */
	evt_test->set_statsd_port(IPV4_PORT_SERVER + 1);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL  ===========================*/

	int32_t client_socket_fd = 0;
	int32_t server_socket_fd = 0;
	struct sockaddr_in client_addr = {0};
	struct sockaddr_in server_addr = {0};
	evt_test->connect_ipv4_client_to_server(&client_socket_fd, &client_addr, &server_socket_fd, &server_addr);

	/* Send a message to the server */
	const unsigned data_len = DEFAULT_SNAPLEN * 2;
	char buf[data_len] = "simple message";
	uint32_t sendto_flags = 0;
	int64_t sent_bytes = syscall(__NR_sendto, client_socket_fd, buf, data_len, sendto_flags, (struct sockaddr *)&server_addr, sizeof(server_addr));
	assert_syscall_state(SYSCALL_SUCCESS, "sendto", sent_bytes, NOT_EQUAL, -1);

	/* Cleaning phase */
	syscall(__NR_shutdown, server_socket_fd, 2);
	syscall(__NR_shutdown, client_socket_fd, 2);
	syscall(__NR_close, server_socket_fd);
	syscall(__NR_close, client_socket_fd);

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->set_do_dynamic_snaplen(false);

	evt_test->assert_event_presence();

	/* we need to clean the values after we read our event because the kernel module
	 * flushes the ring buffers when we change this config.
	 */
	evt_test->set_statsd_port(0);

	if(HasFatalFailure())
	{
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)sent_bytes);

	/* Parameter 2: data (type: PT_BYTEBUF)*/
	evt_test->assert_bytebuf_param(2, buf, DEFAULT_SNAPLEN);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}
#endif
