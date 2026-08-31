/* Regression tests for IPC framing and 2026 temperature conversions. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/Morfeas_RPi_Hat/Morfeas_RPi_Hat.h"
#include "../src/Morfeas_opc_ua/Morfeas_handlers_nodeset.h"
#include "../src/Morfeas_SDAQ/Morfeas_SDAQ_measurement.h"

static int checks, failures;

#define CHECK(condition, message) \
	do { checks++; if(condition) printf("PASS: %s\n", message); \
	else { failures++; fprintf(stderr, "FAIL: %s\n", message); } } while(0)

static void test_ipc_rx(void)
{
	int fd[2];
	IPC_message sent = {0}, received = {0};
	CHECK(pipe(fd) == 0, "pipe opens for IPC framing checks");
	sent.SDAQ_meas.IPC_msg_type = IPC_SDAQ_meas;
	CHECK(write(fd[1], &sent, sizeof(sent)) == (ssize_t)sizeof(sent), "full IPC frame is written");
	CHECK(IPC_msg_RX(fd[0], &received) == IPC_SDAQ_meas, "full known IPC frame is accepted");
	CHECK(write(fd[1], &sent, sizeof(sent) - 1) == (ssize_t)sizeof(sent) - 1, "partial IPC frame is written");
	CHECK(IPC_msg_RX(fd[0], &received) == 0, "partial IPC frame is rejected");
	sent.SDAQ_meas.IPC_msg_type = Morfeas_IPC_MAX_type + 1;
	CHECK(write(fd[1], &sent, sizeof(sent)) == (ssize_t)sizeof(sent), "unknown IPC frame is written");
	CHECK(IPC_msg_RX(fd[0], &received) == 0, "unknown IPC message type is rejected");
	close(fd[0]);
	close(fd[1]);
}

static void test_temperatures(void)
{
	CHECK(fabsf(Morfeas_hat_temperature_celsius(100) - 48.0f) < 0.0001f,
		"MAX9611 raw shunt temperature converts to Celsius");
	CHECK(fabsf(Morfeas_hat_temperature_celsius(-25) + 12.0f) < 0.0001f,
		"negative MAX9611 temperature preserves sign in Celsius");
	CHECK(fabsf(Morfeas_cpu_temp_millicelsius_to_celsius(42750.0f) - 42.75f) < 0.0001f,
		"CPU sysfs millidegrees convert to Celsius");
}

static void prepare_one_cycle(struct SDAQ_info_entry *entry, SDAQ_meas_msg *msg,
	unsigned short timestamp, unsigned char status)
{
	memset(msg, 0, sizeof(*msg));
	msg->Amount_of_channels = 1;
	msg->SDAQ_channel_meas[0].timestamp = timestamp;
	msg->SDAQ_channel_meas[0].status = status;
	msg->SDAQ_channel_meas[0].meas = 17.5f;
	entry->SDAQ_Channels_cycle_seen[0] = 1;
	SDAQ_prepare_cycle_measurements(entry, msg);
}

static void test_sdaq_stall(void)
{
	struct SDAQ_info_entry entry = {0};
	SDAQ_meas_msg msg;
	unsigned short timestamps[1] = {0};
	unsigned char initialized[1] = {0}, stalls[1] = {0}, seen[1] = {0};
	entry.SDAQ_Channels_last_timestamp = timestamps;
	entry.SDAQ_Channels_timestamp_initialized = initialized;
	entry.SDAQ_Channels_stall_cycles = stalls;
	entry.SDAQ_Channels_cycle_seen = seen;

	prepare_one_cycle(&entry, &msg, 7, 0);
	CHECK(msg.SDAQ_channel_meas[0].meas == 17.5f && initialized[0], "first timestamp starts stall tracking without changing measurement");
	prepare_one_cycle(&entry, &msg, 7, 0);
	CHECK(msg.SDAQ_channel_meas[0].meas == 17.5f && stalls[0] == 1, "one repeated timestamp remains below the stall threshold");
	prepare_one_cycle(&entry, &msg, 7, 0);
	CHECK(msg.SDAQ_channel_meas[0].meas == MORFEAS_MEAS_ERROR_STALL && stalls[0] == 2, "second repeated timestamp maps SDAQ measurement to -903");
	prepare_one_cycle(&entry, &msg, 8, 0);
	CHECK(msg.SDAQ_channel_meas[0].meas == 17.5f && stalls[0] == 0, "new timestamp clears SDAQ stall state");
	prepare_one_cycle(&entry, &msg, 8, 1 << No_sensor);
	CHECK(stalls[0] == 0, "explicit no-sensor status never becomes a stall");
}

int main(void)
{
	test_ipc_rx();
	test_temperatures();
	test_sdaq_stall();
	printf("\n%d checks, %d passed, %d failed\n", checks, checks - failures, failures);
	return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
