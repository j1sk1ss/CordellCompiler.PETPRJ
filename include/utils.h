#ifndef UTILS_H_
#define UTILS_H_

#define CONDITIONAL_CASE(option, condition, ...) \
    case (option):                               \
        if ((condition)) {                       \
            __VA_ARGS__                          \
        } else {                                 \
            break;                               \
        }

#endif