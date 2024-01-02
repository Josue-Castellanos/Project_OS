#include "str_manip.h"

// Length of string
uint16_t strLength(char* string) {

    uint16_t i = 1;

    while(string[i++]);

    return --i;
}

// Checks two strings
uint8_t strEqual(char* string1, char* string2) {
    uint8_t result = 1;
    uint8_t size = strLength(string1);

    if ( size != strLength(string2)) {
        // print_str("\nERR: sizes are not the same");
        result = 0;
    }

    else {
        uint8_t i = 0;

        for (i; i <= size; i++) {
            if(string1[i] != string2[i]) {
                // print_str("\nERR: strings do not match");
                result = 0;
            }
        }
    }
    return result;
}

void strncpy(char *dest, char *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        dest[i] = src[i];
    }
}

char* strChr(char* str, char ch) {
    while (*str != '\0') {
        if (*str == ch) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

// Function to convert an integer to a string for print
void int_to_str(uint16_t num, char* buffer, size_t buffer_size) {
    // Handle the case of 0 separately
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    // Initialize variables
    uint8_t i = 0;
    uint8_t is_negative = 0;

    // Handle negative numbers
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    // Build the string in reverse order
    while (num > 0 && i < buffer_size - 1) {
        buffer[i] = '0' + num % 10;
        num /= 10;
        i++;
    }

    // Add '-' for negative numbers
    if (is_negative && i < buffer_size - 1) {
        buffer[i] = '-';
        i++;
    }

    // Null-terminate the string
    buffer[i] = '\0';

    // Reverse the string
    uint8_t start = 0;
    uint8_t end = i - 1;
    while (start < end) {
        // Swap characters
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;

        // Move indices towards the center
        start++;
        end--;
    }
}