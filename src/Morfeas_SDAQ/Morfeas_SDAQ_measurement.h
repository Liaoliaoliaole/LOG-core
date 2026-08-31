#ifndef MORFEAS_SDAQ_MEASUREMENT_H
#define MORFEAS_SDAQ_MEASUREMENT_H

struct SDAQ_info_entry;
struct SDAQ_measure_msg_struct;

/* Normalise one received SDAQ measurement cycle before it is sent to OPC-UA. */
void SDAQ_prepare_cycle_measurements(struct SDAQ_info_entry *sdaq_node,
	struct SDAQ_measure_msg_struct *ipc_meas);

#endif
