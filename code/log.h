//
// Logging utility
// (c) Marcus Larsson
//




//
// Declarations
//

struct Log {
    FILE *file = nullptr;
    b32 print_log = false;
    b32 flush_immediately = true;
};

static b32 open_log(Log *log);
static void close_log(Log *log);
static void log_error(Log *log, char const *string, u32 value, char const *file = nullptr, char const *function = nullptr, u32 line = 0);
static void log_error_str(Log *log, char const *string, char const *error_string, char const *file = nullptr, char const *function = nullptr, u32 line = 0);
static void log_str(Log *log, char const *string);
static void log_current_time(Log *log);
static void close_log(Log *log);

#define LOG_ERROR(log, string, error_value)   log_error(log, string, error_value, __FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR_STR(log, string, error_str) log_error_str(log, string, error_str, __FILE__, __FUNCTION__, __LINE__)




//
// Implementation
//

static b32 open_log(Log *log) {
    b32 result = false;

    if (log) {
        errno_t error = fopen_s(&log->file, "log.txt", "wb");
        if (error != 0) {
            
        }
        else {
            result = true;
            fprintf(log->file, " ________  ___  ___  ________  ________  ___       _______                  _____ ______   ________  ________      \n");
            fprintf(log->file, "|\\   __  \\|\\  \\|\\  \\|\\_____  \\|\\_____  \\|\\  \\     |\\  ___ \\                |\\   _ \\  _   \\|\\   __  \\|\\   ___  \\    \n");
            fprintf(log->file, "\\ \\  \\|\\  \\ \\  \\\\\\  \\\\|___/  /|\\|___/  /\\ \\  \\    \\ \\   __/|   ____________\\ \\  \\\\\\__\\ \\  \\ \\  \\|\\  \\ \\  \\\\ \\  \\   \n");
            fprintf(log->file, " \\ \\   ____\\ \\  \\\\\\  \\   /  / /    /  / /\\ \\  \\    \\ \\  \\_|/__|\\____________\\ \\  \\\\|__| \\  \\ \\   __  \\ \\  \\\\ \\  \\  \n");
            fprintf(log->file, "  \\ \\  \\___|\\ \\  \\\\\\  \\ /  /_/__  /  /_/__\\ \\  \\____\\ \\  \\_|\\ \\|____________|\\ \\  \\    \\ \\  \\ \\  \\ \\  \\ \\  \\\\ \\  \\ \n");
            fprintf(log->file, "   \\ \\__\\    \\ \\_______\\\\________\\\\________\\ \\_______\\ \\_______\\              \\ \\__\\    \\ \\__\\ \\__\\ \\__\\ \\__\\\\ \\__\\\n");
            fprintf(log->file, "    \\|__|     \\|_______|\\|_______|\\|_______|\\|_______|\\|_______|               \\|__|     \\|__|\\|__|\\|__|\\|__| \\|__|\n");
            fprintf(log->file, "\n\n");
            fprintf(log->file, "     A puzzle-homage to Pac-man\n");
            fprintf(log->file, "     Created by Marcus Larsson 2020\n\n");
            
            time_t timer;
            time(&timer);            
            tm local_time;
            u32 time_result = localtime_s(&local_time, &timer);
            if (time_result == 0) {
                fprintf(log->file, "(%04d-%02d-%02d, %02d:%02d:%02d) log-file created.\n",
                        1900 + local_time.tm_year, 1 + local_time.tm_mon, local_time.tm_mday,
                        local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
            }
            else {
                fprintf(log->file, "!ERROR: failed to get the current date and time, error: %d\n", time_result);
            }            
        }

        if (log->flush_immediately) {
            fflush(log->file);
        }
    }

    return result;
}


static void log_current_time(Log *log) {
    if (log) {
        time_t timer;
        time(&timer);            
        tm local_time;
        u32 time_result = localtime_s(&local_time, &timer);
        if (time_result == 0) {
            fprintf(log->file, "(%02d:%02d:%02d) ",
                    local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
        }
        else {
            fprintf(log->file, "!_ERROR: failed to get the current date and time, error: %d\n", time_result);
        }
    }
}


static void log_str(Log *log, char const *string) {
    if (log && string) {
        log_current_time(log);
        fprintf(log->file, "%s\n", string);
    }

    if (log->print_log || !log) {
        printf(string);
    }

    if (log->flush_immediately) {
        fflush(log->file);
    }
}


static void log_error(Log *log, char const *string, u32 error_value, char const *file, char const *function, u32 line) {
    if (log) {
        fprintf(log->file, "!_");
        log_current_time(log);

        if (file && function && line) {
            fprintf(log->file, "Error in %s in %s at line %u, %s (%u)\n", file, function, line, string, error_value);
        }
        else {
            fprintf(log->file, "Error, %s (%u)\n", string, error_value);
        }

        if (log->flush_immediately) {
            fflush(log->file);
        }
    }

    if (log->print_log || !log) {
        printf("Error in %s in %s at line %u, %s (%u)\n", file, function, line, string, error_value);
    }
}


static void log_error_str(Log *log, char const *string, char const *error_string, char const *file, char const *function, u32 line) {
    if (log) {
        fprintf(log->file, "!_");
        log_current_time(log);

        if (file && function && line) {
            if (error_string) {
                fprintf(log->file, "Error in %s in %s at line %u, %s (%s)\n", file, function, line, string, error_string);
            }
            else {
                fprintf(log->file, "Error in %s in %s at line %u, %s\n", file, function, line, string);
            }
        }
        else {
            fprintf(log->file, "Error, %s (%s)\n", string, error_string);
        }

        if (log->flush_immediately) {
            fflush(log->file);
        }
    }

    if (log->print_log || !log) {
        printf("Error in %s in %s at line %u, %s (%s)\n", file, function, line, string, error_string);
    }
}


static void close_log(Log *log) {
    if (log && log->file) {
        fclose(log->file);
    }
}
