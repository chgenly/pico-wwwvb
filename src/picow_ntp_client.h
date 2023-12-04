bool ntp_start(void(*progress)(int p));
bool ntp_ask_for_time(struct tm* utc);
void ntp_end();
