void print_time_us(int64_t t64);
void print_absolute_time(absolute_time_t abstime);
void print_date_time(time_t time);
void print_data_time(double fseconds_since_1970);
int is_leap_year(int year);
int day_of_year(int day, int month, int year);
int day_of_week(int day, int month, int year);
int is_daylight_savings_time(time_t time);
