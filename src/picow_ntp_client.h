bool ntp_start(void(*progress)(int p));
bool ntp_ask_for_time(time_t* utc);
void ntp_end();
