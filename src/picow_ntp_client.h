bool ntp_start(void(*progress)(int p));
bool ntp_ask_for_time(double* futc_seconds_since_1970);
void ntp_end();
